#ifndef NET_HPP
#define NET_HPP

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
#include <memory>
#include <string>

namespace asio = boost::asio;
namespace ssl = asio::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;
using tcp_resolve_results = boost::asio::ip::basic_resolver<tcp>::results_type;

class WebSocketException : public std::exception {
private:
  std::string m_message;

public:
  WebSocketException(const std::string &message) : m_message(message) {}
  WebSocketException(const char *message) : m_message(message) {}

  const char *what() const noexcept override { return m_message.c_str(); }
};

class WebSocket {
public:
  WebSocket(bool useSsl, ssl::context::method context_ssl);
  ~WebSocket();

  int connect_to_server(const std::string &host, int port,
                        const std::string &target);

  std::string read_buffer();

private:
  const tcp_resolve_results _resolve_host(const std::string &host, int port);
  tcp::endpoint _connect(const tcp_resolve_results &results);
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

  beast::flat_buffer m_buffer;
};

class Net {
public:
  static std::unique_ptr<WebSocket> init_websocket(bool useSsl = false);
};

#endif // NET_HPP
