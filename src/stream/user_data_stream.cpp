#include "user_data_stream.hpp"

#include "net/net.hpp"
#include "nlohmann/json_fwd.hpp"
#include "stream/stream.hpp"
#include "utils/constants.hpp"
#include "utils/crypto.hpp"
#include "utils/dotenv.hpp"
#include "utils/json.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>

// TODO: create stream of userData, think about userData controller
int UserDataStreamQueryBuilder::s_requestId = 0;

NetError UserDataStream::connect_to_websocket() {
  NetError wsErr = Stream::_connect_to_websocket(
      std::string(Data::WS_API_HOST), Data::WS_PORT_MAIN,
      std::string(Data::WS_USERDATA_TARGET));

  // If the connection main port failed,
  // try again with backup (https) port
  if (wsErr.hasError()) {
    wsErr.reset();
    wsErr = Stream::_connect_to_websocket(
        std::string(Data::WS_API_HOST), Data::HTTPS_PORT,
        std::string(Data::WS_USERDATA_TARGET));
  }

  return wsErr;
}

NetError UserDataStream::execute_query(const JSONQuery &query) {
  if (!UserDataStreamQueryBuilder::is_query_valid(query)) {
    return NetError(NetErrorType::INVALID_QUERY_ERR,
                    "Query is not UserDataQuery!");
  }

  return Stream::_execute_query(query);
}

// UserDataStreamQueryBuilder
UserDataStreamQueryBuilder::UserDataStreamQueryBuilder(
    USER_DATA_STREAM_METHOD method)
    : m_method(method) {
  m_methodStr = _stream_method_to_str();
  ++s_requestId;

  // Query init
  m_query.set_value(std::string(METHOD), m_methodStr);
  m_query.set_value(std::string(ID), std::to_string(s_requestId));
}

std::string UserDataStreamQueryBuilder::_stream_method_to_str() const {
  return std::string(METHOD_STRINGS[(int)m_method]);
}

std::optional<JSONQuery> UserDataStreamQueryBuilder::commit() {
  // Timestamp is required only for session.logon
  if (m_method == USER_DATA_STREAM_METHOD::SESSION_LOGON) {
    _add_current_timestamp();
    _add_rsa_signature();
    // Merge params subquery with main query
    _add_params_query();
  }

  if (m_lastError.empty() && _is_query_valid()) {
    return m_query;
  }
  std::ostringstream ss;
  ss << "\nCommit unsuccessful due to error: " << m_lastError;
  Log::log_err(ss.str());
  return std::nullopt;
}

UserDataStreamQueryBuilder &
UserDataStreamQueryBuilder::add_apiKey(const std::string &apikey) {
  _add_to_params(std::string(PARAM_APIKEY), apikey);

  m_apiKey = apikey;

  return *this;
}

void UserDataStreamQueryBuilder::_add_to_params(const std::string &key,
                                                const std::string &value) {
  if (value.empty()) {
    m_lastError = "Value is empty for method: " + m_methodStr;
    return;
  }

  if (m_method == USER_DATA_STREAM_METHOD::SESSION_LOGON) {
    m_params_query.set_value(key, value);
  } else {
    m_lastError = "Illegal option 'add_to_params' for method: " + m_methodStr;
  }
}

void UserDataStreamQueryBuilder::_add_current_timestamp() {
  auto now = std::chrono::system_clock::now();
  uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch())
                        .count();
  m_timestamp = now_ms;
  m_params_query.set_value(std::string(PARAM_TIMESTAMP), now_ms);
}

void UserDataStreamQueryBuilder::_add_params_query() {
  if (m_method == USER_DATA_STREAM_METHOD::SESSION_LOGON) {
    m_query.set_value(std::string(PARAMS), m_params_query.json());
  } else {
    m_lastError =
        "Illegal option 'add_params_query' for method: " + m_methodStr;
  }
}

void UserDataStreamQueryBuilder::_add_rsa_signature() {
  if (m_method == USER_DATA_STREAM_METHOD::SESSION_LOGON) {
    // TODO: Make this flow global
    /*
    bool first_param = true;
    std::ostringstream payload;
    for (auto &[key, value] : m_params_query.json().items()) {
      std::string val_str{};
      if (value.is_string()) {
        val_str = value.get<std::string>();
      } else if (value.is_number_unsigned()) {
        val_str = std::to_string(value.get<uint64_t>());
      } else {
        val_str = "null";
      }

      if (!first_param)
        payload << "&";
      else
        first_param = false;

      payload << key << "=" << value;
    }
    */

    std::string payload =
        "apiKey=" + m_apiKey + "&timestamp=" + std::to_string(m_timestamp);

    std::string private_key =
        Env::getInstance().getenv("BINANCE_READ_PRIVATE_KEY");
    if (private_key.empty())
      return;

    std::string signature = Crypto::sign_ed25519(private_key, payload);

    _add_to_params(std::string(PARAM_SIGNATURE), signature);
  } else {
    m_lastError =
        "Illegal option 'add_rsa_signature' for method: " + m_methodStr;
  }
}

/* static */ bool
UserDataStreamQueryBuilder::is_query_valid(const JSONQuery &query) {
  if (query.is_empty())
    return false;

  std::optional<nlohmann::json> json_method =
      query.get_value(std::string(METHOD));

  USER_DATA_STREAM_METHOD method_type = USER_DATA_STREAM_METHOD::SESSION_LOGON;

  if (!json_method.has_value() || !json_method.value().is_string())
    return false;

  const std::string &method_str =
      json_method.value().get_ref<const std::string &>();
  std::string_view sv = method_str;
  auto it = std::find(METHOD_STRINGS.begin(), METHOD_STRINGS.end(), sv);
  if (it == METHOD_STRINGS.end())
    return false;

  method_type =
      (USER_DATA_STREAM_METHOD)std::distance(METHOD_STRINGS.begin(), it);

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
    if (elem == PARAMS) {
      std::optional<nlohmann::json> json_params =
          query.get_value(std::string(PARAMS));
      if (!json_params.has_value())
        return false;

      for (const auto &param : PARAMS_REQUIREMENTS) {
        if (!JSONQuery::is_key_exists(json_params.value(),
                                      std::string(param))) {
          return false;
        }
      }
    }
  }

  return true;
}

bool UserDataStreamQueryBuilder::_is_query_valid() {
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
    if (elem == PARAMS) {
      for (const auto &param : PARAMS_REQUIREMENTS) {
        if (!m_params_query.is_key_exists(std::string(param))) {
          m_lastError = "Missing required param '" + std::string(param) +
                        "' for method: " + m_methodStr;
          return false;
        }
      }
    }
  }
  return true;
}
