#ifndef USER_DATA_STREAM_CONTROLLER_HPP
#define USER_DATA_STREAM_CONTROLLER_HPP

#include "core/controllers/user_data_stream_utils.hpp"
#include "core/net/net.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/stream/user_data_stream.hpp"
#include "core/utils/queue.hpp"
#include "core/utils/time.hpp"

#include <memory>
#include <pstl/glue_algorithm_defs.h>
#include <string>
#include <vector>

namespace UserData {

class IUserEventListener {
public:
  virtual ~IUserEventListener() = default;
  virtual void onEvent(const ParsedUserData &event) = 0;
};

class UserEventPublisher {
public:
  void subscribe(IUserEventListener *listener) {
    if (!listener)
      return;

    if (std::find(m_listeners.begin(), m_listeners.end(), listener) ==
        m_listeners.end()) {
      m_listeners.push_back(listener);
    }
  }

  void unsubscribe(IUserEventListener *listener) {
    m_listeners.erase(
        std::remove(m_listeners.begin(), m_listeners.end(), listener),
        m_listeners.end());
  }

  void publish(const ParsedUserData &event) {
    auto listenersCopy = m_listeners;

    for (auto *listener : listenersCopy) {
      if (listener)
        listener->onEvent(event);
    }
  }

private:
  std::vector<IUserEventListener *> m_listeners;
};

struct MessageStreams {
  // Main string message queue
  Queue<std::string> msgQueue;

  // Queue for parsed messages
  ObservableQueue<StreamMessage> streamQueue;
};

class UserDataStreamController {
public:
  UserDataStreamController();

  void subscribe_to_publisher(IUserEventListener *listener);
  void unsubscribe_from_publisher(IUserEventListener *listener);

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

  std::unique_ptr<UserEventPublisher> m_eventPublisher;

private:
  static constexpr std::chrono::minutes LISTEN_KEY_EXPIRY_TIME{30};
  static constexpr std::string_view LISTEN_KEY_STR{"listenKey"};

  std::unique_ptr<HttpSessionManager> m_http_session_manager;
  std::unique_ptr<AsyncTimer> m_key_timer;
};

} // namespace UserData

#endif // USER_DATA_STREAM_CONTROLLER_HPP
