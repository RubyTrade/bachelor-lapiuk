#ifndef NET_HPP
#define NET_HPP

#include "nlohmann/json_fwd.hpp"
#include "utils/thread.hpp"
#include <atomic>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using tcp_resolve_results = boost::asio::ip::basic_resolver<tcp>::results_type;
using work_guard_t =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

enum class WSErrorType {
  UNDEFINED = -1,
  NONE = 0,
  RESOLVE_HOST_ERR,
  SSL_HANDSHAKE_ERR,
  WS_HANDSHAKE_ERR,
  TCP_CONNECT_ERR,
  READ_BUFFER_ERR,
  WRITE_BUFFER_ERR
};

class WSError {
public:
  explicit WSError(WSErrorType err, const std::string &msg)
      : m_code((int)err), m_msg(msg), m_res(err) {}

  WSError() : m_code((int)WSErrorType::NONE), m_res(WSErrorType::NONE) {}

  void setError(WSErrorType err, const std::string &msg) {
    m_code = (int)err;
    m_msg = msg;
    m_res = err;
  }

  void reset() {
    m_code = (int)WSErrorType::NONE;
    m_res = WSErrorType::NONE;
    m_msg = "";
  }

  bool hasError() const { return m_res != WSErrorType::NONE; }

  int getCode() const { return m_code; }

  std::string getMessage() const { return m_msg; }

  WSErrorType getError() const { return m_res; }

private:
  int m_code;
  std::string m_msg{};
  WSErrorType m_res{WSErrorType::NONE};
};

class WebSocketException : public std::exception {
private:
  std::string m_message;
  WSError m_wsError;

public:
  WebSocketException(const std::string &message,
                     WSErrorType errType = WSErrorType::UNDEFINED)
      : m_message(message) {
    m_wsError.setError(errType, message);
  }
  WebSocketException(const char *message,
                     WSErrorType errType = WSErrorType::UNDEFINED)
      : m_message(message) {

    m_wsError.setError(errType, message);
  }

  const char *what() const noexcept override { return m_message.c_str(); }
  const WSError getError() const noexcept { return m_wsError; }
};

using MessageHandler = std::function<void(std::string &&)>;

// WebSocket
class WebSocket {
public:
  WebSocket(bool useSsl, ssl::context::method context_ssl);
  ~WebSocket();

  WSError connect_to_server(const std::string &host, int port,
                            const std::string &target = "");

  void async_write_text(const std::string msg);
  void async_write_json(const nlohmann::json json);

  void start_async_read(MessageHandler &&handler);
  void run_io_context();

  void stop_websocket();

private:
  void _set_message_handler(MessageHandler &&handler);
  void _read_async();

  const tcp_resolve_results _resolve_host(const std::string &host, int port);
  tcp::endpoint _tcp_connect(const tcp_resolve_results &results);
  void _ssl_handshake();
  void _websocket_handshake(const std::string &host, const std::string &target);

private:
  bool m_is_ssl;

  // Contexts
  asio::io_context m_io_context;
  ssl::context m_ssl_context;

  tcp::resolver m_resolver;

  // Websocket
  websocket::stream<beast::ssl_stream<tcp::socket>> m_websocket;

  std::optional<work_guard_t> m_work_guard;
  std::atomic_bool m_is_io_running;

  Thread m_io_context_thread;

  beast::flat_buffer m_buffer;

  MessageHandler m_handler; // handler for websocket messages
};

// Net
class Net {
public:
  static std::unique_ptr<WebSocket> init_websocket(bool useSsl = false);
};

#endif // NET_HPP
