#ifndef USER_DATA_STREAM_HPP
#define USER_DATA_STREAM_HPP

#include "net/net.hpp"
#include "stream.hpp"
#include "utils/json.hpp"
#include <cstdint>

/*
 * Session logong method example:
 {
  "id": 1,
  "method": "session.logon",
  "params": {
    "apiKey": "API_KEY",
    "timestamp": 1700000000000,
    "signature": "ED25519_SIGNATURE"
  }
} */
enum class USER_DATA_STREAM_METHOD {
  SESSION_LOGON = 0,
  SESSION_STATUS,
  SESSION_LOGOUT,
  STREAM_SUBSCRIBE,
  STREAM_UNSUBSCRIBE,
  STREAM_SUBSCRIPTIONS
};

class UserDataStreamQueryBuilder {
public:
  explicit UserDataStreamQueryBuilder(USER_DATA_STREAM_METHOD method);
  UserDataStreamQueryBuilder() = delete;

  std::optional<JSONQuery> commit();

  static bool is_query_valid(const JSONQuery &query);

  UserDataStreamQueryBuilder &add_apiKey(const std::string &apikey);

private:
  bool _is_query_valid();
  std::string _stream_method_to_str() const;
  void _add_to_params(const std::string &key, const std::string &value);

  void _add_current_timestamp();
  void _add_rsa_signature();
  void _add_params_query();

private:
  static constexpr short MAX_PROPS_NUM = 3;
  static constexpr std::string_view PARAMS = "params";
  static constexpr std::string_view METHOD = "method";
  static constexpr std::string_view ID = "id";
  static constexpr std::string_view PARAM_APIKEY = "apiKey";
  static constexpr std::string_view PARAM_SIGNATURE = "signature";
  static constexpr std::string_view PARAM_TIMESTAMP = "timestamp";
  static constexpr std::array<std::string_view, 6> METHOD_STRINGS{
      "session.logon",
      "session.status",
      "session.logout",
      "userDataStream.subscribe",
      "userDataStream.unsubscribe",
      "userDataStream.subscriptions"};

  static constexpr std::array<std::string_view, 3> PARAMS_REQUIREMENTS{
      PARAM_APIKEY, PARAM_SIGNATURE, PARAM_TIMESTAMP};

  static constexpr std::array<
      std::pair<USER_DATA_STREAM_METHOD,
                std::array<std::string_view, MAX_PROPS_NUM>>,
      6>
      METHOD_REQUIREMENTS{{
          {USER_DATA_STREAM_METHOD::SESSION_LOGON, {METHOD, ID, PARAMS}},
          {USER_DATA_STREAM_METHOD::SESSION_STATUS, {METHOD, ID, ""}},
          {USER_DATA_STREAM_METHOD::SESSION_LOGOUT, {METHOD, ID, ""}},
          {USER_DATA_STREAM_METHOD::STREAM_SUBSCRIBE, {METHOD, ID, ""}},
          {USER_DATA_STREAM_METHOD::STREAM_UNSUBSCRIBE, {METHOD, ID, ""}},
          {USER_DATA_STREAM_METHOD::STREAM_SUBSCRIPTIONS, {METHOD, ID, ""}},
      }};

private:
  USER_DATA_STREAM_METHOD m_method;
  std::string m_methodStr;

  static int s_requestId;

  JSONQuery m_query;
  JSONQuery m_params_query;

  uint64_t m_timestamp = 0;
  std::string m_apiKey{};

  std::string m_lastError{};
};

// User Data Stream
class UserDataStream : public Stream {
public:
  NetError connect_to_websocket() override;

  NetError execute_query(const JSONQuery &query) override;
};

#endif // USER_DATA_STREAM_HPP
