#include "stream.hpp"

#include <sstream>

#include "core/utils/log.hpp"

// TODO: add check if ws is running

// Stream
Stream::Stream(Queue<std::string> &msgQueue)
    : m_webSocket(Net::init_websocket()), m_msgQueue(msgQueue) {}

NetError Stream::_connect_to_websocket(const std::string &host, int port,
                                       const std::string &target) {
  NetError wsErr = m_webSocket->connect_to_server(host, port, target);

  std::ostringstream ss;
  if (wsErr.hasError()) {
    ss << "\nConnection unsuccessful: " << "errCode: " << (int)wsErr.getCode()
       << " message: " << wsErr.getMessage();

    Log::log_err(ss.str());
  } else {
    ss << "\nSuccessfully connected to: " << port << ":" << host << target;

    Log::log(ss.str());
  }

  // Run context for async methods
  m_webSocket->run_io_context();

  return wsErr;
}

void Stream::disconnect_from_websocket() { _disconnect_from_websocket(); }

void Stream::_disconnect_from_websocket() { m_webSocket->stop_websocket(); }

NetError Stream::_execute_query(const JSONQuery &query) {
  NetError wsErr;
  if (!query.is_empty()) {
    m_webSocket->async_write_json(query.json());
    return wsErr;
  }

  wsErr.setError(NetErrorType::WRITE_BUFFER_ERR, "The query is empty!");
  return wsErr;
}

void Stream::start_listening(std::function<void()> &&onError_handler) {
  m_webSocket->start_async_read(
      [this](std::string &&msg) { m_msgQueue.push_message(std::move(msg)); },
      std::move(onError_handler));
}
