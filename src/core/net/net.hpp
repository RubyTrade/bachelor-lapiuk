#ifndef NET_HPP
#define NET_HPP

#include "core/utils/log.hpp"
#include "core/utils/thread.hpp"
#include "nlohmann/json_fwd.hpp"
#include <algorithm>
#include <atomic>
#include <boost/asio/execution/prefer_only.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/resolver_base.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
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
namespace http = beast::http;
using tcp = asio::ip::tcp;
using tcp_resolve_results = boost::asio::ip::basic_resolver<tcp>::results_type;
using work_guard_t =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

enum class NetErrorType {
  UNDEFINED = -1,
  NONE = 0,
  RESOLVE_HOST_ERR,
  SSL_HANDSHAKE_ERR,
  WS_HANDSHAKE_ERR,
  TCP_CONNECT_ERR,
  READ_BUFFER_ERR,
  WRITE_BUFFER_ERR,
  SHUTDOWN_ERR,
  INVALID_SOCKET_ERR,
  INVALID_QUERY_ERR
};

class NetError {
public:
  explicit NetError(NetErrorType err, const std::string &msg)
      : m_code((int)err), m_msg(msg), m_res(err) {}

  NetError() : m_code((int)NetErrorType::NONE), m_res(NetErrorType::NONE) {}

  void setError(NetErrorType err, const std::string &msg) {
    m_code = (int)err;
    m_msg = msg;
    m_res = err;
  }

  void reset() {
    m_code = (int)NetErrorType::NONE;
    m_res = NetErrorType::NONE;
    m_msg = "";
  }

  bool hasError() const { return m_res != NetErrorType::NONE; }

  int getCode() const { return m_code; }

  std::string getMessage() const { return m_msg; }

  NetErrorType getError() const { return m_res; }

private:
  int m_code;
  std::string m_msg{};
  NetErrorType m_res{NetErrorType::NONE};
};

class NetworkException : public std::exception {
private:
  std::string m_message;
  NetError m_wsError;

public:
  NetworkException(const std::string &message,
                   NetErrorType errType = NetErrorType::UNDEFINED)
      : m_message(message) {
    m_wsError.setError(errType, message);
  }
  NetworkException(const char *message,
                   NetErrorType errType = NetErrorType::UNDEFINED)
      : m_message(message) {

    m_wsError.setError(errType, message);
  }

  const char *what() const noexcept override { return m_message.c_str(); }
  const NetError getError() const noexcept { return m_wsError; }
};

// ---------------------------- WebSocket ------------------------------

using MessageHandler = std::function<void(std::string &&)>;
using OnErrorHandler = std::function<void()>;

// WebSocket
class WebSocket {
public:
  WebSocket(bool useSsl, ssl::context::method context_ssl);
  ~WebSocket();

  NetError connect_to_server(const std::string &host, int port,
                             const std::string &target = "");

  void async_write_text(const std::string msg);
  void async_write_json(const nlohmann::json json);

  void start_async_read(MessageHandler &&handler, OnErrorHandler &&errHandler);
  void run_io_context();

  void stop_websocket();

  bool is_ws_running() const { return m_is_ws_running.load(); }

private:
  void _set_up_websocket();
  void _set_message_handler(MessageHandler &&handler);
  void _set_onError_handler(OnErrorHandler &&errHandler);
  void _read_async();

  const tcp_resolve_results _resolve_host(const std::string &host, int port);
  tcp::endpoint _tcp_connect(const tcp_resolve_results &results);
  void _ssl_handshake(const std::string &host);
  void _websocket_handshake(const std::string &host, const std::string &target);

private:
  bool m_is_ssl;

  // Contexts
  asio::io_context m_io_context;
  ssl::context m_ssl_context;

  tcp::resolver m_resolver;

  // Websocket
  std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>>
      m_websocket;

  std::optional<work_guard_t> m_work_guard;
  std::atomic_bool m_is_io_running{false};
  std::atomic_bool m_is_ws_running{false};

  Thread m_io_context_thread;

  beast::flat_buffer m_buffer;

  MessageHandler m_handler;    // handler for websocket messages
  OnErrorHandler m_errHandler; // handler for websocket errors
};

// ---------------------------- Http ------------------------------

// Only supported methods listed
enum class HttpMethod { GET, POST, DELETE, PUT };

// Http Client
class HttpRequest {
public:
  HttpRequest(asio::io_context &io_context, ssl::context &ssl_context);

  ~HttpRequest() = default;

  NetError do_request(
      const HttpMethod &method, const std::string &host, int port,
      const std::string &target, const std::string &body, bool useSsl,
      std::string &response_buffer,
      const std::vector<std::pair<std::string, std::string>> &headers = {});

private:
  http::verb _to_verb(const HttpMethod &method);

  const tcp_resolve_results _resolve_host(const std::string &host, int port);

  http::request<http::string_body> _prepare_request(
      const HttpMethod &method, const std::string &host,
      const std::string &target, const std::string &body,
      const std::vector<std::pair<std::string, std::string>> &headers);

  template <typename StreamT>
  tcp::endpoint _tcp_connect(StreamT &stream,
                             const tcp_resolve_results &results);

  template <typename StreamT>
  void _ssl_handshake(StreamT &stream, const std::string &host);

  template <typename StreamT>
  void _write(StreamT &stream, http::request<http::string_body> req);

  template <typename StreamT> std::string _read(StreamT &stream);

  template <typename StreamT> void _shutdown(StreamT &stream);

  template <typename StreamT>
  NetError
  _do_request(StreamT &stream, const HttpMethod &method,
              const std::string &host, int port, const std::string &target,
              const std::string &body, std::string &response_buffer,
              const std::vector<std::pair<std::string, std::string>> &headers);

private:
  // Contexts
  asio::io_context &m_io_context;
  ssl::context &m_ssl_context;

  tcp::resolver m_resolver;

  bool m_is_shutted_down = false;

  tcp::socket m_http_socket;               // main http stream socket
  ssl::stream<tcp::socket> m_https_socket; // main https stream socket
};

struct HttpQuery {
private:
  HttpMethod m_method;
  std::string m_host{};
  int m_port{0};
  std::string m_target = "/";
  std::string m_body{};
  bool m_useSsl = true;
  std::vector<std::pair<std::string, std::string>> m_headers{};

public:
  const HttpMethod &method() const { return m_method; }
  const std::string &host() const { return m_host; }
  int port() const { return m_port; }
  const std::string &target() const { return m_target; }
  const std::string &body() const { return m_body; }
  bool is_ssl() const { return m_useSsl; }
  const auto &headers() const { return m_headers; }

  void setHost(const std::string &h) { m_host = h; }
  void setPort(const int &p) { m_port = p; }
  void setTarget(const std::string &t) { m_target = t; }
  void setBody(const std::string &b) { m_body = b; }
  void setUseSsl(const bool &u) { m_useSsl = u; }
  void
  setHeaders(const std::vector<std::pair<std::string, std::string>> &headers) {
    m_headers = headers;
  }

public:
  HttpQuery(HttpMethod method) : m_method(method) {}
  HttpQuery() = delete;
};

class HttpQueryBuilder {
public:
  HttpQueryBuilder(HttpMethod method) : m_query(method) {}
  HttpQueryBuilder() = delete;

  HttpQueryBuilder &setHost(const std::string &host) {
    if (host.find(".") == std::string::npos) {
      m_lastError = "Specified Host is invalid!";
      return *this;
    }
    m_query.setHost(host);
    return *this;
  }

  HttpQueryBuilder &setHost(const std::string_view &sw_host) {
    std::string host = std::string(sw_host);
    if (host.find(".") == std::string::npos) {
      m_lastError = "Specified Host is invalid!";
      return *this;
    }
    m_query.setHost(host);
    return *this;
  }

  HttpQueryBuilder &setPort(const int &port) {
    if (port < 0 || port > 65535) {
      m_lastError = "Specified Port is invalid!";
      return *this;
    }
    m_query.setPort(port);
    return *this;
  }
  HttpQueryBuilder &setTarget(const std::string &target) {
    m_query.setTarget(target);
    return *this;
  }
  HttpQueryBuilder &setTarget(const std::string_view &sw_target) {
    std::string target = std::string(sw_target);
    m_query.setTarget(target);
    return *this;
  }
  HttpQueryBuilder &setBody(const std::string &body) {
    m_query.setBody(body);
    return *this;
  }
  HttpQueryBuilder &setBody(const std::string_view &sw_body) {
    std::string body = std::string(sw_body);
    m_query.setBody(body);
    return *this;
  }
  HttpQueryBuilder &setUseSsl(const bool &useSsl) {
    m_query.setUseSsl(useSsl);
    return *this;
  }
  HttpQueryBuilder &
  setHeaders(const std::vector<std::pair<std::string, std::string>> &headers) {
    if (!headers.empty()) {
      for (const auto &header : headers) {
        if (!_is_valid_header(header.first)) {
          m_lastError = "Header key " + header.first + " is invalid!";
          return *this;
        }
      }
    }
    m_query.setHeaders(headers);
    return *this;
  }

  std::optional<HttpQuery> commit();

private:
  bool _is_valid_header(std::string_view key) {
    return std::find(HEADER_KEYS.begin(), HEADER_KEYS.end(), key) !=
           HEADER_KEYS.end();
  }

private:
  HttpQuery m_query;

  std::string m_lastError;

  static constexpr std::array<std::string_view, 4> HEADER_KEYS{
      "X-MBX-APIKEY", "Content-Type", "Content-Length", "Connection"};
};

class HttpSessionManager {
public:
  HttpSessionManager(ssl::context::method context_ssl);

  NetError do_request(const HttpQuery &httpQuery, std::string &response_buffer);

private:
  // Contexts
  asio::io_context m_io_context;
  ssl::context m_ssl_context;
};

// ---------------------------- Network ------------------------------

// Net
class Net {
public:
  static std::unique_ptr<WebSocket> init_websocket(bool useSsl = false);
  static std::unique_ptr<HttpSessionManager> init_http_session_manager();
};

#endif // NET_HPP
