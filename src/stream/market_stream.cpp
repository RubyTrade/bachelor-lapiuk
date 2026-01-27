#include "market_stream.hpp"
#include "net/net.hpp"
#include "utils/json.hpp"
#include "utils/log.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "utils/constants.hpp"

int MarketStreamQueryBuilder::s_requestId = 0;

NetError MarketStream::connect_to_websocket() {
  NetError wsErr = Stream::_connect_to_websocket(
      std::string(Data::WS_HOST), Data::WS_PORT_MAIN,
      std::string(Data::WS_DEFAULT_TARGET));

  // If the connection main port failed,
  // try again with backup (https) port
  if (wsErr.hasError()) {
    wsErr.reset();
    wsErr = Stream::_connect_to_websocket(std::string(Data::WS_HOST),
                                          Data::HTTPS_PORT,
                                          std::string(Data::WS_DEFAULT_TARGET));
  }

  return wsErr;
}

NetError MarketStream::execute_query(const JSONQuery &query) {
  if (!MarketStreamQueryBuilder::is_query_valid(query)) {
    return NetError(NetErrorType::INVALID_QUERY_ERR,
                    "Query is not MarketStreamQuery!");
  }

  return Stream::_execute_query(query);
}

// MarketStreamQueryBuilder
MarketStreamQueryBuilder::MarketStreamQueryBuilder(MARKET_STREAM_METHOD method)
    : m_method(method) {
  m_methodStr = _stream_method_to_str();
  ++s_requestId;

  // Query init
  m_query.set_value(std::string(METHOD), m_methodStr);
  m_query.set_value(std::string(ID), std::to_string(s_requestId));
}

std::string MarketStreamQueryBuilder::_stream_method_to_str() const {
  return std::string(METHOD_STRINGS[(int)m_method]);
}

std::optional<JSONQuery> MarketStreamQueryBuilder::commit() {
  // Only for GET_PROPERTY the single param is available: combined
  if (m_method == MARKET_STREAM_METHOD::GET_PROPERTY) {
    m_query.add_to_array(std::string(PARAMS), std::string("combined"));
  }

  if (m_lastError.empty() && _is_query_valid()) {
    return m_query;
  }
  std::ostringstream ss;
  ss << "\nCommit unsuccessful due to error: " << m_lastError;
  Log::log_err(ss.str());
  return std::nullopt;
}

/* static */ bool
MarketStreamQueryBuilder::is_query_valid(const JSONQuery &query) {
  if (query.is_empty())
    return false;

  std::optional<nlohmann::json> json_method =
      query.get_value(std::string(METHOD));

  MARKET_STREAM_METHOD method_type = MARKET_STREAM_METHOD::SUBSCRIBE;

  if (!json_method.has_value() || !json_method.value().is_string())
    return false;

  const std::string &method_str =
      json_method.value().get_ref<const std::string &>();
  std::string_view sv = method_str;
  auto it = std::find(METHOD_STRINGS.begin(), METHOD_STRINGS.end(), sv);
  if (it == METHOD_STRINGS.end())
    return false;

  method_type = (MARKET_STREAM_METHOD)std::distance(METHOD_STRINGS.begin(), it);

  const auto &get_elem = [&]() {
    for (const auto &elem : METHOD_REQUIREMENTS) {
      if (elem.first == method_type) {
        return elem.second;
      }
    }
    // Impossible
    return METHOD_REQUIREMENTS[0].second;
  }();

  const std::array<std::string_view, MAX_PROPS_NUM> &required_props = get_elem;

  for (const auto &elem : required_props) {
    if (!elem.empty() && !query.is_key_exists(std::string(elem))) {
      return false;
    }
  }

  return true;
}

bool MarketStreamQueryBuilder::_is_query_valid() {
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

void MarketStreamQueryBuilder::_add_to_params(const std::string &value) {
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

MarketStreamQueryBuilder &
MarketStreamQueryBuilder::add_trade_symbol(const std::string &symbol) {
  _add_to_params(symbol + "@trade");

  return *this;
}

MarketStreamQueryBuilder &
MarketStreamQueryBuilder::add_aggTrade_symbol(const std::string &symbol) {
  _add_to_params(symbol + "@aggTrade");

  return *this;
}

MarketStreamQueryBuilder &
MarketStreamQueryBuilder::add_deffDepth_symbol(const std::string &symbol,
                                               bool fast_update /*= true*/) {
  std::string fast_update_flag = "";
  if (!fast_update) {
    fast_update_flag = "@1000ms";
  }
  _add_to_params(symbol + "@depth" + fast_update_flag);

  return *this;
}

MarketStreamQueryBuilder &
MarketStreamQueryBuilder::add_partDepth_symbol(const std::string &symbol,
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

MarketStreamQueryBuilder &
MarketStreamQueryBuilder::add_bookTicker_symbol(const std::string &symbol) {
  _add_to_params(symbol + "@bookTicker");

  return *this;
}

MarketStreamQueryBuilder &
MarketStreamQueryBuilder::set_combined_property(bool combined_flag /*= true*/) {
  if (m_method == MARKET_STREAM_METHOD::SET_PROPERTY) {
    m_query.add_to_array(std::string(PARAMS), std::string("combined"));
    m_query.add_to_array(std::string(PARAMS), combined_flag);
  } else {
    m_lastError =
        "Illegal option 'set_combined_property' for method: " + m_methodStr;
  }

  return *this;
}
