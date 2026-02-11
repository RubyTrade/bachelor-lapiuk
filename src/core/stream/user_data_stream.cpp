#include "user_data_stream.hpp"

#include "core/net/net.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/dotenv.hpp"
#include "core/utils/json.hpp"
#include "core/utils/time.hpp"
#include "stream.hpp"

#include "nlohmann/json_fwd.hpp"

#include <memory>
#include <optional>
#include <string>
#include <sys/types.h>

// TODO: add listenKey validity check

UserDataStream::UserDataStream()
    : m_http_session_manager(Net::init_http_session_manager()),
      m_key_timer(std::make_unique<AsyncTimer>()) {}

NetError UserDataStream::connect_to_websocket() {
  std::string listenKey = _create_listenKey();

  m_key_timer->start_recurring(LISTEN_KEY_EXPIRY_TIME,
                               [this]() { this->_update_listenKey(); });

  NetError wsErr = Stream::_connect_to_websocket(
      std::string(Data::WS_HOST), Data::HTTPS_PORT,
      std::string(Data::WS_USERDATA_TARGET) + "/" + listenKey);
  return wsErr;
}

std::string UserDataStream::_create_listenKey() {
  std::string listenKey{};
  std::string buffer{};
  std::string apiKey =
      Env::getInstance().getenv(std::string(Data::BINANCE_READ_APIKEY_ENV));

  std::optional<HttpQuery> query =
      HttpQueryBuilder(HttpMethod::POST)
          .setHost(Data::API_HOST)
          .setPort(Data::HTTPS_PORT)
          .setTarget(std::string(Data::API_DEFAULT_TARGET) +
                     std::string(LISTEN_KEY_STR))
          .setHeaders({{std::string(Data::Header::APIKEY), apiKey}})
          .commit();

  if (query.has_value()) {
    m_http_session_manager->do_request(query.value(), buffer);
  }

  if (buffer.empty())
    return {};

  std::optional<nlohmann::json> json_val =
      JSONQuery(buffer).get_value(std::string(LISTEN_KEY_STR));

  if (json_val.has_value() && json_val.value().is_string()) {
    listenKey = json_val.value().get_ref<const std::string &>();
  }

  return listenKey;
}

void UserDataStream::_update_listenKey() {
  std::string buffer{};
  std::string apiKey =
      Env::getInstance().getenv(std::string(Data::BINANCE_READ_APIKEY_ENV));

  std::optional<HttpQuery> query =
      HttpQueryBuilder(HttpMethod::PUT)
          .setHost(Data::API_HOST)
          .setPort(Data::HTTPS_PORT)
          .setTarget(std::string(Data::API_DEFAULT_TARGET) +
                     std::string(LISTEN_KEY_STR))
          .setHeaders({{std::string(Data::Header::APIKEY), apiKey}})
          .commit();

  if (query.has_value()) {
    m_http_session_manager->do_request(query.value(), buffer);
  }
}

NetError UserDataStream::execute_query(const JSONQuery &query) {
  // UserDataStream is READ only!
  return NetError(NetErrorType::INVALID_QUERY_ERR,
                  "UserDataStream is READ only!");
}
