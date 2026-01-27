#ifndef STREAM_HPP
#define STREAM_HPP

#include "net/net.hpp"
#include "utils/json.hpp"
#include "utils/queue.hpp"

// Stream
class Stream {
public:
  Stream();
  virtual ~Stream() = default;

  virtual NetError connect_to_websocket() = 0;
  virtual NetError execute_query(const JSONQuery &query) = 0;

  // Temp method
  void start_listening();
  void start_reading();

protected:
  NetError _connect_to_websocket(const std::string &host, int port,
                                 const std::string &target = "");

  NetError _execute_query(const JSONQuery &query);

private:
  Queue<std::string> m_msgQueue;

protected:
  std::unique_ptr<WebSocket> m_webSocket; // main stream socket
};

#endif // STREAM_HPP
