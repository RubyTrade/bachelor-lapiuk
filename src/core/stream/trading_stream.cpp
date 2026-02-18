#include "trading_stream.hpp"

#include "core/net/net.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/crypto.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/json.hpp"
#include "stream.hpp"

// TODO: think about userData controller
// TODO: think about userData via REST API
// TODO: add leverage control

int TradingStreamQueryBuilder::s_requestId = 0;

NetError TradingStream::connect_to_websocket() {
  NetError wsErr = Stream::_connect_to_websocket(
      std::string(Data::WS_TRADING_HOST), Data::WS_PORT_MAIN,
      std::string(Data::WS_TRADING_API_TARGET));

  if (!wsErr.hasError()) {
    std::optional<JSONQuery> auth_query =
        TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD::SESSION_LOGON)
            .commit();

    if (auth_query) {
      wsErr = execute_query(auth_query.value());
    }
  }

  return wsErr;
}

NetError TradingStream::execute_query(const JSONQuery &query) {
  if (!TradingStreamQueryBuilder::is_query_valid(query)) {
    return NetError(NetErrorType::INVALID_QUERY_ERR,
                    "Query is not MarketStreamQuery!");
  }

  return Stream::_execute_query(query);
}

// TradingStreamQueryBuilder
TradingStreamQueryBuilder::TradingStreamQueryBuilder(
    USER_DATA_STREAM_METHOD method) {
  setMethod(method);
}

TradingStreamQueryBuilder &
TradingStreamQueryBuilder::setMethod(USER_DATA_STREAM_METHOD method) {
  m_method = method;
  m_methodStr = type_to_str(METHOD_NAMES, m_method);

  return *this;
}

std::optional<JSONQuery> TradingStreamQueryBuilder::commit() {
  _init_query();

  // Timestamp is required only for session.logon
  if (is_params_required(m_method)) {
    _add_apiKey();
    _add_current_timestamp();
    _add_rsa_signature();

    // Merge params subquery with main query
    _add_params_query();
  }

  if (m_lastError.empty() && _is_query_valid()) {
    JSONQuery tmp = m_query;

    // Clear up after successfull commit
    _cleanup();

    return tmp;
  }
  std::ostringstream ss;
  ss << "\nCommit unsuccessful due to error: " << m_lastError;
  Log::log_err(ss.str());

  _cleanup();

  return std::nullopt;
}

TradingStreamQueryBuilder &
TradingStreamQueryBuilder::add_borderless_params(const JSONQuery &query) {
  if (is_params_required(m_method)) {
    std::map<std::string, JSONValue> items = query.get_map_of_items();

    for (auto &elem : items) {
      std::string valueStr = [&elem]() {
        std::string str{};
        if (auto val = std::get_if<std::string>(&elem.second)) {
          str = *val;
        } else if (auto val = std::get_if<bool>(&elem.second)) {
          str = (*val) ? "true" : "false";
        } else if (auto val = std::get_if<nlohmann::json>(&elem.second)) {
          str = (*val).get<std::string>();
        } else if (auto val = std::get_if<uint64_t>(&elem.second)) {
          str = std::to_string(*val);
        } else if (auto val = std::get_if<int>(&elem.second)) {
          str = std::to_string(*val);
        }
        return str;
      }();

      _add_to_params(elem.first, valueStr);
    }

  } else {
    m_lastError =
        "Illegal option 'add_borderless_params' for method: " + m_methodStr;
  }

  return *this;
}

void TradingStreamQueryBuilder::_init_query() {
  m_query.set_value(std::string(METHOD), m_methodStr);
  m_query.set_value(std::string(ID), std::to_string(++s_requestId));
}

void TradingStreamQueryBuilder::_cleanup() {
  m_lastError = "";
  m_query = JSONQuery();
  m_params_query = JSONQuery();
}

void TradingStreamQueryBuilder::_add_to_params(const std::string &key,
                                               const std::string &value) {
  if (value.empty()) {
    m_lastError = "Value is empty for method: " + m_methodStr;
    return;
  }

  if (is_params_required(m_method)) {
    m_params_query.set_value(key, value);
  } else {
    m_lastError = "Illegal option 'add_to_params' for method: " + m_methodStr;
  }
}

void TradingStreamQueryBuilder::_add_apiKey() {
  std::string apiKey{};
  if (is_boundless_params_required(m_method)) {
    apiKey =
        Env::getInstance().getenv(std::string(Data::BINANCE_WRITE_APIKEY_ENV));
  } else if (is_params_required(m_method)) {
    apiKey =
        Env::getInstance().getenv(std::string(Data::BINANCE_READ_APIKEY_ENV));
  }

  if (apiKey.empty())
    return;

  m_params_query.set_value(std::string(PARAM_APIKEY), apiKey);
}

void TradingStreamQueryBuilder::_add_current_timestamp() {
  auto now = std::chrono::system_clock::now();
  uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch())
                        .count();
  m_params_query.set_value(std::string(PARAM_TIMESTAMP), now_ms);
}

void TradingStreamQueryBuilder::_add_params_query() {
  if (is_params_required(m_method)) {
    m_query.set_value(std::string(PARAMS), m_params_query.json());
  } else {
    m_lastError =
        "Illegal option 'add_params_query' for method: " + m_methodStr;
  }
}

void TradingStreamQueryBuilder::_add_rsa_signature() {
  if (is_params_required(m_method)) {
    std::map<std::string, JSONValue> items = m_params_query.get_map_of_items();

    bool first_param = true;
    std::ostringstream payload;

    for (auto &elem : items) {
      if (!first_param) {
        payload << "&";
      } else {
        first_param = false;
      }

      std::string valueStr = [&elem]() {
        std::string str{};
        if (auto val = std::get_if<std::string>(&elem.second)) {
          str = *val;
        } else if (auto val = std::get_if<bool>(&elem.second)) {
          str = (*val) ? "true" : "false";
        } else if (auto val = std::get_if<nlohmann::json>(&elem.second)) {
          str = (*val).get<std::string>();
        } else if (auto val = std::get_if<uint64_t>(&elem.second)) {
          str = std::to_string(*val);
        } else if (auto val = std::get_if<int>(&elem.second)) {
          str = std::to_string(*val);
        }
        return str;
      }();

      payload << elem.first << "=" << valueStr;
    }

    std::string private_key =
        Env::getInstance().getenv(std::string(Data::BINANCE_PK_ENV));
    if (private_key.empty())
      return;

    std::string signature = Crypto::sign_ed25519(private_key, payload.str());

    _add_to_params(std::string(PARAM_SIGNATURE), signature);
  } else {
    m_lastError =
        "Illegal option 'add_rsa_signature' for method: " + m_methodStr;
  }
}

/* static */ bool TradingStreamQueryBuilder::is_params_required(
    const USER_DATA_STREAM_METHOD &method) {
  uint16_t m = static_cast<uint16_t>(method);

  return ((m & 0xFF00) >= uint16_t(USER_DATA_METHOD_GROUP::PARAMS_ONLY));
}

/* static */ bool TradingStreamQueryBuilder::is_boundless_params_required(
    const USER_DATA_STREAM_METHOD &method) {
  uint16_t m = static_cast<uint16_t>(method);

  return ((m & 0xFF00) >= uint16_t(USER_DATA_METHOD_GROUP::BOUNDLESS_PARAMS));
}

/* static */ bool
TradingStreamQueryBuilder::is_query_valid(const JSONQuery &query) {
  if (query.is_empty())
    return false;

  std::optional<nlohmann::json> json_method =
      query.get_value(std::string(METHOD));

  USER_DATA_STREAM_METHOD method_type = USER_DATA_STREAM_METHOD::INVALID_METHOD;

  if (!json_method.has_value() || !json_method.value().is_string())
    return false;

  const std::string &method_str =
      json_method.value().get_ref<const std::string &>();

  method_type = str_to_type(METHOD_NAMES, method_str);

  if (method_type == USER_DATA_STREAM_METHOD::INVALID_METHOD)
    return false;

  for (const auto &elem : METHOD_REQUIREMENTS) {
    if (!query.is_key_exists(std::string(elem))) {
      return false;
    }
  }

  if (is_params_required(method_type)) {
    std::optional<nlohmann::json> json_params =
        query.get_value(std::string(PARAMS));

    if (!query.is_key_exists(std::string(PARAMS)) || !json_params.has_value())
      return false;

    for (const auto &elem : PARAMS_REQUIREMENTS) {
      if (!JSONQuery::is_key_exists(json_params.value(), std::string(elem))) {
        return false;
      }
    }
  }
  return true;
}

bool TradingStreamQueryBuilder::_is_query_valid() {
  if (m_query.is_empty()) {
    m_lastError = "The query is empty!";
    return false;
  }

  if (m_method == USER_DATA_STREAM_METHOD::INVALID_METHOD) {
    m_lastError = "Selected method is invalid!";
    return false;
  }

  for (const auto &elem : METHOD_REQUIREMENTS) {
    if (!m_query.is_key_exists(std::string(elem))) {
      m_lastError = "Missing required property '" + std::string(elem) +
                    "' for method: " + m_methodStr;
      return false;
    }
  }

  if (is_params_required(m_method)) {
    if (!m_query.is_key_exists(std::string(PARAMS))) {
      m_lastError =
          "Missing param in main query for method that requires params: " +
          m_methodStr;
      return false;
    }
    for (const auto &elem : PARAMS_REQUIREMENTS) {
      if (!m_params_query.is_key_exists(std::string(elem))) {
        m_lastError = "Missing required param '" + std::string(elem) +
                      "' for method: " + m_methodStr;
        return false;
      }
    }
  }

  return true;
}

std::optional<JSONQuery> ParametersBuilder::commit() {
  if (m_lastError.empty()) {
    JSONQuery tmp = m_params_query;

    // Clear up after successfull commit
    _cleanup();

    return tmp;
  }

  std::ostringstream ss;
  ss << "\nCommit unsuccessful due to error: " << m_lastError;
  Log::log_err(ss.str());

  _cleanup();

  return std::nullopt;
}

ParametersBuilder &ParametersBuilder::add_symbol(const std::string &symbol) {
  _add_to_params("symbol", symbol);

  return *this;
}

void ParametersBuilder::_cleanup() {
  m_lastError = "";
  m_params_query = JSONQuery();
}

ParametersBuilder &
ParametersBuilder::add_positionSide(const POSITION_SIDE &pos_side) {
  std::string side_str = type_to_str(POSITION_SIDE_STR, pos_side);
  _add_to_params("positionSide", side_str);

  return *this;
}

ParametersBuilder &ParametersBuilder::add_price(const Fixed &price) {
  if (price > Fixed(0))
    _add_to_params("price", price.to_string());

  return *this;
}

ParametersBuilder &ParametersBuilder::add_quantity(const Fixed &quantity) {
  if (quantity > Fixed(0))
    _add_to_params("quantity", quantity.to_string());

  return *this;
}

ParametersBuilder &ParametersBuilder::add_side(const ORDER_SIDE &side) {
  std::string side_str = type_to_str(ORDER_SIDE_STR, side);
  _add_to_params("side", side_str);

  return *this;
}

ParametersBuilder &
ParametersBuilder::add_timeInForce(const TIME_IN_FORCE &timeInForce) {
  std::string tif = type_to_str(TIME_IN_FORCE_STR, timeInForce);
  _add_to_params("timeInForce", tif);

  return *this;
}

ParametersBuilder &ParametersBuilder::add_type(const ORDER_TYPE &type) {
  std::string type_str = type_to_str(ORDER_TYPE_STR, type);
  _add_to_params("type", type_str);

  return *this;
}

// Algo params
ParametersBuilder &ParametersBuilder::add_algoType(const ORDER_TYPE &type) {
  std::string type_str = type_to_str(ORDER_TYPE_STR, type);
  _add_to_params("algoType", type_str);

  return *this;
}

ParametersBuilder &
ParametersBuilder::add_newOrderRespType(const std::string &type) {
  _add_to_params("newOrderRespType", type);

  return *this;
}

ParametersBuilder &ParametersBuilder::add_triggerPrice(const Fixed &price) {
  if (price > Fixed(0))
    _add_to_params("triggerPrice", price.to_string());

  return *this;
}

ParametersBuilder &ParametersBuilder::add_orderId(int64_t orderId) {
  _add_to_params("orderId", std::to_string(orderId));

  return *this;
}

ParametersBuilder &
ParametersBuilder::add_priceMatch(const std::string &priceMatch) {
  _add_to_params("priceMatch", priceMatch);

  return *this;
}

ParametersBuilder &ParametersBuilder::add_origType(const ORDER_TYPE &type) {
  std::string type_str = type_to_str(ORDER_TYPE_STR, type);
  _add_to_params("origType", type_str);

  return *this;
}

void ParametersBuilder::_add_to_params(const std::string &key,
                                       const std::string &value) {
  if (value.empty() || key.empty()) {
    m_lastError = "Value or Key is empty!";
    return;
  }

  m_params_query.set_value(key, value);
}
