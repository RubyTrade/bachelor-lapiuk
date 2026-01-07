#include "market_stream.hpp"
#include "utils/log.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include "utils/constants.hpp"

int StreamQueryBuilder::s_requestId = 0;

// Market Stream
MarketStream::MarketStream() : m_webSocket(Net::init_websocket()) {}

WSError MarketStream::connect_to_websocket() {
  WSError wsErr = m_webSocket->connect_to_server(
      std::string(Data::WS_HOST), Data::WS_PORT_MAIN,
      std::string(Data::DEFAULT_TARGET));

  std::ostringstream ss;
  if (wsErr.hasError()) {
    ss << "\nConnection unsuccessful: " << "errCode: " << (int)wsErr.getCode()
       << " message: " << wsErr.getMessage();

    Log::log_err(ss.str());
  } else {
    ss << "\nSuccessfully connected to: " << Data::WS_PORT_MAIN << ":"
       << Data::WS_HOST << Data::DEFAULT_TARGET;

    Log::log(ss.str());
  }

  // Run context for async methods
  m_webSocket->run_io_context();

  return wsErr;
}

WSError MarketStream::execute_query(const JSONQuery &query) {
  WSError wsErr;
  if (!query.is_empty()) {
    m_webSocket->async_write_json(query.json());
    return wsErr;
  }

  wsErr.setError(WSErrorType::WRITE_BUFFER_ERR, "The query is empty!");
  return wsErr;
}

// Temp method
void MarketStream::start_listening() {
  m_webSocket->start_async_read(
      [this](std::string &&msg) { m_msgQueue.push_message(std::move(msg)); });
}

void MarketStream::start_reading() {
  while (true) {
    std::string out_msg;
    bool res = m_msgQueue.pop_message(out_msg);
    if (res) {
      Log::log("Read: " + out_msg);
    }
  }
}

// StreamQueryBuilder
StreamQueryBuilder::StreamQueryBuilder(MARKET_STREAM_METHOD method)
    : m_method(method) {
  m_methodStr = _stream_method_to_str();
  ++s_requestId;

  // Query init
  m_query.set_value(std::string(METHOD), m_methodStr);
  m_query.set_value(std::string(ID), std::to_string(s_requestId));
}

std::string StreamQueryBuilder::_stream_method_to_str() const {
  return std::string(METHOD_STRINGS[(int)m_method]);
}

std::optional<JSONQuery> StreamQueryBuilder::commit() {
  // Only for GET_PROPERTY the single param is available: combined
  if (m_method == MARKET_STREAM_METHOD::GET_PROPERTY) {
    m_query.add_to_array(std::string(PARAMS), std::string("combined"));
  }

  if (m_lastError.empty() && _is_query_valid()) {
    return m_query;
  }
  std::cerr << "\nCommit unsuccessful due to error: " << m_lastError;
  return std::nullopt;
}

bool StreamQueryBuilder::_is_query_valid() {
  if (m_query.is_empty()) {
    m_lastError = "The query is empty!";
    return false;
  }

  const auto &get_elem = [&]() {
    for (const auto &elem : METHOD_REQUIREMENTS) {
      if (elem.first == m_method) {
        return elem.second;
      }
    }
    // Impossible
    return METHOD_REQUIREMENTS[0].second;
  }();

  const std::array<std::string_view, MAX_PROPS_NUM> &required_props = get_elem;

  for (const auto &elem : required_props) {
    if (!elem.empty() && !m_query.is_key_exists(std::string(elem))) {
      m_lastError = "Missing required property '" + std::string(elem) +
                    "' for method: " + m_methodStr;
      return false;
    }
  }
  return true;
}

void StreamQueryBuilder::_add_to_params(const std::string &value) {
  if (value.empty()) {
    m_lastError = "Value is empty for method: " + m_methodStr;
    return;
  }

  if (m_method == MARKET_STREAM_METHOD::SUBSCRIBE ||
      m_method == MARKET_STREAM_METHOD::UNSUBSCRIBE) {
    m_query.add_to_array(std::string(PARAMS), value);
  } else {
    m_lastError = "Illegal option 'add_symbol' for method: " + m_methodStr;
  }
}

StreamQueryBuilder &
StreamQueryBuilder::add_trade_symbol(const std::string &symbol) {
  _add_to_params(symbol + "@trade");

  return *this;
}

StreamQueryBuilder &
StreamQueryBuilder::add_aggTrade_symbol(const std::string &symbol) {
  _add_to_params(symbol + "@aggTrade");

  return *this;
}

StreamQueryBuilder &
StreamQueryBuilder::add_deffDepth_symbol(const std::string &symbol,
                                         bool fast_update /*= true*/) {
  std::string fast_update_flag = "";
  if (!fast_update) {
    fast_update_flag = "@1000ms";
  }
  _add_to_params(symbol + "@depth" + fast_update_flag);

  return *this;
}

StreamQueryBuilder &
StreamQueryBuilder::add_partDepth_symbol(const std::string &symbol,
                                         DEPTH_LEVELS level,
                                         bool fast_update /*= true*/) {
  std::string fast_update_flag = "";
  if (!fast_update) {
    fast_update_flag = "@1000ms";
  }
  _add_to_params(symbol + "@depth" + std::to_string((int)level) +
                 fast_update_flag);

  return *this;
}

StreamQueryBuilder &
StreamQueryBuilder::add_bookTicker_symbol(const std::string &symbol) {
  _add_to_params(symbol + "@bookTicker");

  return *this;
}

StreamQueryBuilder &
StreamQueryBuilder::set_combined_property(bool combined_flag /*= true*/) {
  if (m_method == MARKET_STREAM_METHOD::SET_PROPERTY) {
    m_query.add_to_array(std::string(PARAMS), std::string("combined"));
    m_query.add_to_array(std::string(PARAMS), combined_flag);
  } else {
    m_lastError =
        "Illegal option 'set_combined_property' for method: " + m_methodStr;
  }

  return *this;
}
