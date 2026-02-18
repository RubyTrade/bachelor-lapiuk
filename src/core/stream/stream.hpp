#ifndef STREAM_HPP
#define STREAM_HPP

#include "core/net/net.hpp"
#include "core/utils/json.hpp"
#include "core/utils/queue.hpp"

// TODO: change streams query id's to be UUID type

// Stream
class Stream {
public:
  Stream(Queue<std::string> &msgQueue);
  virtual ~Stream() = default;

  virtual NetError connect_to_websocket() = 0;
  virtual NetError execute_query(const JSONQuery &query) = 0;

  void start_listening();

protected:
  NetError _connect_to_websocket(const std::string &host, int port,
                                 const std::string &target = "");

  NetError _execute_query(const JSONQuery &query);

private:
  Queue<std::string> &m_msgQueue;

protected:
  std::unique_ptr<WebSocket> m_webSocket; // main stream socket
};

#endif // STREAM_HPP
