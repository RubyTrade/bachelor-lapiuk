#include "net.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/impl/connect.hpp>
#include <boost/asio/ip/basic_resolver.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/system/detail/error_code.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

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

const tcp_resolve_results WebSocket::_resolve_host(const std::string &host,
                                                   int port) {
  boost::system::error_code error_code;
  const tcp_resolve_results results =
      m_resolver.resolve(host, std::to_string(port), error_code);

  if (error_code) {
    throw WebSocketException("resolve_host error: " + error_code.message());
    return tcp_resolve_results{};
  }

  if (results.empty()) {
    throw WebSocketException("resolve_host error: results are empty!");
    return tcp_resolve_results{};
  }

  return results;
}

tcp::endpoint WebSocket::_connect(const tcp_resolve_results &results) {
  boost::system::error_code error_code;
  tcp::endpoint endpoint =
      asio::connect(m_websocket.next_layer().next_layer(), results, error_code);

  if (error_code) {
    throw WebSocketException("connect error: " + error_code.message());
    return tcp::endpoint{};
  }
  return endpoint;
}

void WebSocket::_ssl_handshake() {
  boost::system::error_code error_code;
  m_websocket.next_layer().handshake(ssl::stream_base::client, error_code);
  if (error_code) {
    throw WebSocketException("ssl_handshake error: " + error_code.message());
    return;
  }
}

void WebSocket::_websocket_handshake(const std::string &host,
                                     const std::string &target) {
  boost::system::error_code error_code;
  m_websocket.handshake(host, target, error_code);
  if (error_code) {
    throw WebSocketException("websocket_handshake error: " +
                             error_code.message());
    return;
  }
}

int WebSocket::connect_to_server(const std::string &host, int port,
                                 const std::string &target) {
  try {
    // DNS Resolver
    const tcp_resolve_results results = _resolve_host(host, port);

    // TCP Connection
    _connect(results);

    // SSL Handshake
    _ssl_handshake();

    // WebSocket Handshake
    _websocket_handshake(host, target);

  } catch (const WebSocketException &ws_exception) {
    std::cerr << "\nWebSocket Exception: " << ws_exception.what();
    return -1;
  } catch (const std::exception &exception) {
    std::cerr << "\nUknown exception: " << exception.what();
    return -1;
  }

  return 0;
}

std::string WebSocket::read_buffer() {
  m_websocket.read(m_buffer);
  std::string data = beast::buffers_to_string(m_buffer.data());
  m_buffer.consume(m_buffer.size());
  return data;
}

// Net
/* static */ std::unique_ptr<WebSocket> Net::init_websocket(bool useSsl) {
  return std::make_unique<WebSocket>(useSsl, ssl::context::tlsv12_client);
}
