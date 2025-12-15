#include "net.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/impl/connect.hpp>
#include <boost/asio/ip/basic_resolver.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/system/detail/error_code.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include <nlohmann/json.hpp>
#include <utility>

#include "utils/log.hpp"
#include "utils/thread.hpp"

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

WebSocket::~WebSocket() {}

void WebSocket::_set_message_handler(MessageHandler &&handler) {
  m_handler = std::move(handler);
}

void WebSocket::_read_async() {
  m_websocket.async_read(
      m_buffer, [this](boost::system::error_code ec, std::size_t bytes_sent) {
        if (ec) {
          // TODO: maybe handle this error
          Log::log_err("\nRead error: " + ec.message());
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
    throw WebSocketException("resolve_host error: " + error_code.message(),
                             WSErrorType::RESOLVE_HOST_ERR);
    return tcp_resolve_results{};
  }

  if (results.empty()) {
    throw WebSocketException("resolve_host error: results are empty!",
                             WSErrorType::RESOLVE_HOST_ERR);
    return tcp_resolve_results{};
  }

  return results;
}

tcp::endpoint WebSocket::_tcp_connect(const tcp_resolve_results &results) {
  boost::system::error_code error_code;
  tcp::endpoint endpoint =
      asio::connect(m_websocket.next_layer().next_layer(), results, error_code);

  if (error_code) {
    throw WebSocketException("connect error: " + error_code.message(),
                             WSErrorType::TCP_CONNECT_ERR);
    return tcp::endpoint{};
  }
  return endpoint;
}

void WebSocket::_ssl_handshake() {
  boost::system::error_code error_code;
  m_websocket.next_layer().handshake(ssl::stream_base::client, error_code);
  if (error_code) {
    throw WebSocketException("ssl_handshake error: " + error_code.message(),
                             WSErrorType::SSL_HANDSHAKE_ERR);
    return;
  }
}

void WebSocket::_websocket_handshake(const std::string &host,
                                     const std::string &target) {
  boost::system::error_code error_code;
  m_websocket.handshake(host, target, error_code);
  if (error_code) {
    throw WebSocketException("websocket_handshake error: " +
                                 error_code.message(),
                             WSErrorType::WS_HANDSHAKE_ERR);
    return;
  }
}

WSError WebSocket::connect_to_server(const std::string &host, int port,
                                     const std::string &target) {
  WSError wsErr;

  try {
    // DNS Resolver
    const tcp_resolve_results results = _resolve_host(host, port);

    // TCP Connection
    _tcp_connect(results);

    // SSL Handshake
    _ssl_handshake();

    // WebSocket Handshake
    _websocket_handshake(host, target);

  } catch (const WebSocketException &ws_exception) {
    std::cerr << "\nWebSocket Exception: " << ws_exception.what();

    return ws_exception.getError();
  } catch (const std::exception &exception) {
    std::cerr << "\nUknown exception: " << exception.what();

    wsErr.setError(WSErrorType::UNDEFINED, exception.what());
    return wsErr;
  }

  return wsErr;
}

void WebSocket::async_write_text(const std::string msg) {
  // TODO: handle the error using WSError

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
  // TODO: handle the error using WSError

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

// Net
/* static */ std::unique_ptr<WebSocket> Net::init_websocket(bool useSsl) {
  return std::make_unique<WebSocket>(useSsl, ssl::context::tlsv12_client);
}
