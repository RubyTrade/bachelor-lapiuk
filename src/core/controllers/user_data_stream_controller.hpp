#ifndef USER_DATA_STREAM_CONTROLLER_HPP
#define USER_DATA_STREAM_CONTROLLER_HPP

#include "core/controllers/user_data_stream_utils.hpp"
#include "core/net/net.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/event_publisher.hpp"
#include "core/utils/queue.hpp"
#include "core/utils/time.hpp"

#include <atomic>
#include <memory>
#include <string>

namespace UserData {

struct MessageStreams {
  Queue<std::string> msgQueue;
  ObservableQueue<StreamMessage> streamQueue;
};

class UserDataStreamController {
public:
  UserDataStreamController();

  void subscribe_to_publisher(IEventListener<ParsedUserData> *listener);
  void unsubscribe_from_publisher(IEventListener<ParsedUserData> *listener);

private:
  void _reconnect();

  void _start_listen_thread();
  void _start_read_thread();

  void _start_buffer_reading();

  void _parse_msg(const std::string &&msg);

  std::string _create_listenKey();
  void _update_listenKey();

private:
  std::unique_ptr<UserDataStream> m_userDataStream;

  std::unique_ptr<Thread> m_listenThread;
  std::unique_ptr<Thread> m_readThread;

  std::unique_ptr<MessageStreams> m_userDataMsgQueues;
  std::unique_ptr<ObservableQueue<ParsedUserData>> m_parsedStreamData;

  std::unique_ptr<EventPublisher<ParsedUserData>> m_eventPublisher;

  std::atomic_bool m_is_stream_running{false};

  static constexpr std::chrono::minutes LISTEN_KEY_EXPIRY_TIME{30};
  static constexpr std::string_view LISTEN_KEY_STR{"listenKey"};

  std::unique_ptr<HttpSessionManager> m_http_session_manager;
  std::unique_ptr<AsyncTimer> m_key_timer;
};

} // namespace UserData

#endif // USER_DATA_STREAM_CONTROLLER_HPP
