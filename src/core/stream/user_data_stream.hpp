#ifndef USER_DATA_STREAM_HPP
#define USER_DATA_STREAM_HPP

#include "core/net/net.hpp"
#include "core/utils/json.hpp"
#include "core/utils/time.hpp"
#include "stream.hpp"
#include <chrono>
#include <memory>
#include <string_view>

// User Data Stream
class UserDataStream : public Stream {
public:
  UserDataStream();

  NetError connect_to_websocket() override;

  NetError execute_query(const JSONQuery &query) override;

private:
  std::string _create_listenKey();
  void _update_listenKey();

private:
  static constexpr std::chrono::minutes LISTEN_KEY_EXPIRY_TIME{30};
  static constexpr std::string_view LISTEN_KEY_STR{"listenKey"};

  std::unique_ptr<HttpSessionManager> m_http_session_manager;
  std::unique_ptr<AsyncTimer> m_key_timer;
};

#endif // USER_DATA_STREAM_HPP
