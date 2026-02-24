#ifndef USER_DATA_STREAM_CONTROLLER_HPP
#define USER_DATA_STREAM_CONTROLLER_HPP

#include "core/controllers/user_data_stream_utils.hpp"
#include "core/net/net.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/queue.hpp"
#include "core/utils/time.hpp"

#include <memory>
#include <string>

namespace UserData {

struct MessageStreams {
  // Main string message queue
  Queue<std::string> msgQueue;

  // Queue for parsed messages
  ObservableQueue<StreamMessage> streamQueue;
};

class UserDataStreamController {
public:
  UserDataStreamController();

private:
  void _start_listen_thread();
  void _start_read_thread();

  void _start_buffer_reading();

  NetError _do_session_logon();

  void _parse_msg(const std::string &&msg);

private:
  std::string _create_listenKey();
  void _update_listenKey();

private:
  // Main stream
  std::unique_ptr<UserDataStream> m_userDataStream;

  // Threads
  std::unique_ptr<Thread> m_listenThread;
  std::unique_ptr<Thread> m_readThread;

  // Main string message queue
  std::unique_ptr<MessageStreams> m_userDataMsgQueues;
  std::unique_ptr<Queue<ParsedUserData>> m_parsedStreamData;

private:
  static constexpr std::chrono::minutes LISTEN_KEY_EXPIRY_TIME{30};
  static constexpr std::string_view LISTEN_KEY_STR{"listenKey"};

  std::unique_ptr<HttpSessionManager> m_http_session_manager;
  std::unique_ptr<AsyncTimer> m_key_timer;
};

} // namespace UserData

#endif // USER_DATA_STREAM_CONTROLLER_HPP
