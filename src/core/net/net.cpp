#include "net.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/impl/connect.hpp>
#include <boost/asio/ip/basic_resolver.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/query.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/system/detail/error_code.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include <nlohmann/json.hpp>
#include <type_traits>
#include <utility>

#include "core/utils/constants.hpp"
#include "core/utils/log.hpp"
#include "core/utils/thread.hpp"

// WebSocket
WebSocket::WebSocket(bool useSsl, ssl::context::method context_ssl)
    : m_is_ssl(useSsl), m_ssl_context(context_ssl), m_resolver(m_io_context),
      m_websocket(m_io_context, m_ssl_context) {
  // Turn Off/On SSL verification
  if (m_is_ssl)
    m_websocket.next_layer().set_verify_mode(ssl::verify_peer);
  else
    m_websocket.next_layer().set_verify_mode(ssl::verify_none);
}

WebSocket::~WebSocket() { stop_websocket(); }

void WebSocket::_set_message_handler(MessageHandler &&handler) {
  m_handler = std::move(handler);
}

void WebSocket::_read_async() {
  m_websocket.async_read(
      m_buffer, [this](boost::system::error_code ec, std::size_t bytes_sent) {
        if (ec) {
          // TODO: maybe handle this error
          Log::log_err("\nRead error: " + ec.message());
          if (ec.message().find("Operation canceled") != std::string::npos)
            return;
        }

        std::string data = beast::buffers_to_string(m_buffer.data());
        m_buffer.consume(m_buffer.size());

        if (m_handler) {
          m_handler(std::move(data));
        }

        if (m_is_io_running.load())
          _read_async();
      });
}

const tcp_resolve_results WebSocket::_resolve_host(const std::string &host,
                                                   int port) {
  boost::system::error_code error_code;
  const tcp_resolve_results results =
      m_resolver.resolve(host, std::to_string(port), error_code);

  if (error_code) {
    throw NetworkException("resolve_host error: " + error_code.message(),
                           NetErrorType::RESOLVE_HOST_ERR);
    return tcp_resolve_results{};
  }

  if (results.empty()) {
    throw NetworkException("resolve_host error: results are empty!",
                           NetErrorType::RESOLVE_HOST_ERR);
    return tcp_resolve_results{};
  }

  return results;
}

tcp::endpoint WebSocket::_tcp_connect(const tcp_resolve_results &results) {
  boost::system::error_code error_code;
  tcp::endpoint endpoint =
      asio::connect(m_websocket.next_layer().next_layer(), results, error_code);

  if (error_code) {
    throw NetworkException("connect error: " + error_code.message(),
                           NetErrorType::TCP_CONNECT_ERR);
    return tcp::endpoint{};
  }
  return endpoint;
}

void WebSocket::_ssl_handshake(const std::string &host) {
  boost::system::error_code error_code;
  // Setting up SNI
  if (!SSL_set_tlsext_host_name(m_websocket.next_layer().native_handle(),
                                host.c_str())) {
    throw NetworkException("Failed to set SNI",
                           NetErrorType::SSL_HANDSHAKE_ERR);
  }

  m_websocket.next_layer().handshake(ssl::stream_base::client, error_code);
  if (error_code) {
    throw NetworkException("ssl_handshake error: " + error_code.message(),
                           NetErrorType::SSL_HANDSHAKE_ERR);
    return;
  }
}

void WebSocket::_websocket_handshake(const std::string &host,
                                     const std::string &target) {
  boost::system::error_code error_code;
  m_websocket.handshake(host, target, error_code);
  if (error_code) {
    throw NetworkException("websocket_handshake error: " + error_code.message(),
                           NetErrorType::WS_HANDSHAKE_ERR);
    return;
  }
}

NetError WebSocket::connect_to_server(const std::string &host, int port,
                                      const std::string &target) {
  NetError wsErr;

  try {
    // DNS Resolver
    const tcp_resolve_results results = _resolve_host(host, port);

    // TCP Connection
    _tcp_connect(results);

    // SSL Handshake
    _ssl_handshake(host);

    // WebSocket Handshake
    _websocket_handshake(host, target);

  } catch (const NetworkException &ws_exception) {
    std::cerr << "\nWebSocket Exception: " << ws_exception.what();

    return ws_exception.getError();
  } catch (const std::exception &exception) {
    std::cerr << "\nUnknown exception: " << exception.what();

    wsErr.setError(NetErrorType::UNDEFINED, exception.what());
    return wsErr;
  }

  return wsErr;
}

void WebSocket::async_write_text(const std::string msg) {
  // TODO: handle the error using NetError

  boost::asio::post(m_io_context, [this, msg = std::move(msg)]() mutable {
    boost::system::error_code error_code; // internal ec for this method

    m_websocket.text(true);
    m_websocket.write(boost::asio::buffer(msg), error_code);

    if (error_code) {
      Log::log_err("write_text error: " + error_code.message());
    }
  });
}

void WebSocket::async_write_json(const nlohmann::json json) {
  // TODO: handle the error using NetError

  boost::asio::post(m_io_context, [this, json = std::move(json)]() mutable {
    boost::system::error_code error_code; // internal ec for this method
    m_websocket.write(boost::asio::buffer(json.dump()), error_code);

    if (error_code) {
      Log::log_err("write_json error: " + error_code.message());
    }
  });
}

void WebSocket::start_async_read(MessageHandler &&handler) {
  _set_message_handler(std::move(handler));

  _read_async();
}

void WebSocket::run_io_context() {
  m_is_io_running.store(true);

  m_work_guard.emplace(m_io_context.get_executor());

  m_io_context_thread
      .start([this]() {
        // start async read
        m_io_context.run();
      })
      .detach();
}

void WebSocket::stop_websocket() {
  boost::system::error_code ec;
  m_websocket.close(websocket::close_code::normal, ec);
  if ((ec == boost::asio::error::operation_aborted ||
       ec == websocket::error::closed) &&
      m_is_io_running.load()) {
    m_work_guard.reset();
    m_io_context.stop();

    m_is_io_running.store(false);
  }
}

// Http client
HttpRequest::HttpRequest(asio::io_context &io_context,
                         ssl::context &ssl_context)
    : m_io_context(io_context), m_ssl_context(ssl_context),
      m_resolver(m_io_context), m_http_socket(m_io_context),
      m_https_socket(m_io_context, m_ssl_context) {}

NetError HttpRequest::do_request(
    const HttpMethod &method, const std::string &host, int port,
    const std::string &target, const std::string &body, bool useSsl,
    std::string &response_buffer,
    const std::vector<std::pair<std::string, std::string>> &headers) {
  if (m_is_shutted_down) {
    NetError netErr;
    netErr.setError(NetErrorType::INVALID_SOCKET_ERR,
                    "That http client was already used, create a new one!");
    return netErr;
  }

  if (useSsl) {
    return _do_request(m_https_socket, method, host, port, target, body,
                       response_buffer, headers);
  } else {
    return _do_request(m_http_socket, method, host, port, target, body,
                       response_buffer, headers);
  }
}

template <typename StreamT>
NetError HttpRequest::_do_request(
    StreamT &stream, const HttpMethod &method, const std::string &host,
    int port, const std::string &target, const std::string &body,
    std::string &response_buffer,
    const std::vector<std::pair<std::string, std::string>> &headers) {
  NetError wsErr;

  try {
    // DNS Resolver
    const tcp_resolve_results results = _resolve_host(host, port);

    // TCP Connection
    _tcp_connect(stream, results);

    if constexpr (std::is_same_v<StreamT, ssl::stream<tcp::socket>>) {
      // SSL Handshake
      _ssl_handshake(stream, host);
    }
    // Prepare request
    http::request<http::string_body> req =
        _prepare_request(method, host, target, body, headers);

    // Write
    _write(stream, req);

    // Read
    response_buffer = _read(stream);

    // Shutdown
    _shutdown(stream);

  } catch (const NetworkException &ws_exception) {
    std::cerr << "\nHttpRequest Exception: " << ws_exception.what();

    return ws_exception.getError();
  } catch (const std::exception &exception) {
    std::cerr << "\nUnknown exception: " << exception.what();

    wsErr.setError(NetErrorType::UNDEFINED, exception.what());
    return wsErr;
  }

  return wsErr;
}

http::verb HttpRequest::_to_verb(const HttpMethod &method) {
  switch (method) {
  case HttpMethod::GET:
    return http::verb::get;
  case HttpMethod::POST:
    return http::verb::post;
  case HttpMethod::DELETE:
    return http::verb::delete_;
  case HttpMethod::PUT:
    return http::verb::put;
  }

  // Default value that will never be reached
  return http::verb::get;
}

const tcp_resolve_results HttpRequest::_resolve_host(const std::string &host,
                                                     int port) {
  boost::system::error_code error_code;
  const tcp_resolve_results results =
      m_resolver.resolve(host, std::to_string(port), error_code);

  if (error_code) {
    throw NetworkException("resolve_host error: " + error_code.message(),
                           NetErrorType::RESOLVE_HOST_ERR);
    return tcp_resolve_results{};
  }

  if (results.empty()) {
    throw NetworkException("resolve_host error: results are empty!",
                           NetErrorType::RESOLVE_HOST_ERR);
    return tcp_resolve_results{};
  }

  return results;
}

template <typename StreamT>
tcp::endpoint HttpRequest::_tcp_connect(StreamT &stream,
                                        const tcp_resolve_results &results) {
  boost::system::error_code error_code;
  tcp::endpoint endpoint;
  if constexpr (std::is_same_v<StreamT, ssl::stream<tcp::socket>>) {
    endpoint = asio::connect(stream.next_layer(), results, error_code);
  } else {
    endpoint = asio::connect(stream, results, error_code);
  }

  if (error_code) {
    throw NetworkException("connect error: " + error_code.message(),
                           NetErrorType::TCP_CONNECT_ERR);
    return tcp::endpoint{};
  }
  return endpoint;
}

template <typename StreamT>
void HttpRequest::_ssl_handshake(StreamT &stream, const std::string &host) {
  boost::system::error_code error_code;
  if constexpr (std::is_same_v<StreamT, ssl::stream<tcp::socket>>) {
    // Setting up SNI
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
      throw NetworkException("Failed to set SNI",
                             NetErrorType::SSL_HANDSHAKE_ERR);
    }

    stream.handshake(ssl::stream_base::client, error_code);
  }
  if (error_code) {
    throw NetworkException("ssl_handshake error: " + error_code.message(),
                           NetErrorType::SSL_HANDSHAKE_ERR);
    return;
  }
}

http::request<http::string_body> HttpRequest::_prepare_request(
    const HttpMethod &method, const std::string &host,
    const std::string &target, const std::string &body,
    const std::vector<std::pair<std::string, std::string>> &headers) {

  http::verb http_method = _to_verb(method);

  http::request<http::string_body> req{http_method, target, 11};

  req.set(http::field::host, host);
  req.set(http::field::user_agent, std::string(Data::HTTP_USER_AGENT));

  for (const auto &[k, v] : headers) {
    req.set(k, v);
  }

  req.body() = body;
  req.prepare_payload();

  return req;
}

template <typename StreamT>
void HttpRequest::_write(StreamT &stream,
                         http::request<http::string_body> req) {
  boost::system::error_code ec;

  http::write(stream, req, ec);

  if (ec) {
    throw NetworkException("write error: " + ec.message(),
                           NetErrorType::WRITE_BUFFER_ERR);
  }
}

template <typename StreamT> std::string HttpRequest::_read(StreamT &stream) {
  boost::system::error_code ec;

  beast::flat_buffer buffer;
  http::response<http::string_body> res;

  http::read(stream, buffer, res, ec);

  if (ec) {
    throw NetworkException("read error: " + ec.message(),
                           NetErrorType::READ_BUFFER_ERR);
  }

  return res.body();
}

template <typename StreamT> void HttpRequest::_shutdown(StreamT &stream) {
  boost::system::error_code ec;

  if constexpr (std::is_same_v<StreamT, ssl::stream<tcp::socket>>) {
    stream.shutdown(ec);
    if (ec == boost::asio::ssl::error::stream_truncated) {
      ec.clear();
    }
  } else {
    stream.shutdown(asio::socket_base::shutdown_both, ec);
  }

  if (ec) {
    throw NetworkException("shutdown error: " + ec.message(),
                           NetErrorType::SHUTDOWN_ERR);
  } else {
    m_is_shutted_down = true;
  }
}

std::optional<HttpQuery> HttpQueryBuilder::commit() {
  if (m_lastError.empty()) {
    return m_query;
  }
  std::ostringstream ss;
  ss << "\nCommit unsuccessful due to error: " << m_lastError;
  Log::log_err(ss.str());
  return std::nullopt;
}

HttpSessionManager::HttpSessionManager(ssl::context::method context_ssl)
    : m_ssl_context(context_ssl) {
  m_ssl_context.set_verify_mode(ssl::verify_peer);
  m_ssl_context.set_default_verify_paths();
};

NetError HttpSessionManager::do_request(const HttpQuery &httpQuery,
                                        std::string &response_buffer) {
  HttpRequest httpRequest{m_io_context, m_ssl_context};
  NetError netErr = httpRequest.do_request(
      httpQuery.method(), httpQuery.host(), httpQuery.port(),
      httpQuery.target(), httpQuery.body(), httpQuery.is_ssl(), response_buffer,
      httpQuery.headers());

  return netErr;
}

// Net
/* static */ std::unique_ptr<WebSocket> Net::init_websocket(bool useSsl) {
  return std::make_unique<WebSocket>(useSsl, ssl::context::tlsv12_client);
}

/* static */ std::unique_ptr<HttpSessionManager>
Net::init_http_session_manager() {
  return std::make_unique<HttpSessionManager>(ssl::context::tlsv12_client);
}
