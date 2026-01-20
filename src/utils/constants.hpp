#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string_view>

namespace Data {

inline constexpr std::string_view WS_HOST = "stream.binance.com";
inline constexpr std::string_view WS_API_HOST = "ws-api.binance.com";
inline constexpr std::string_view API_HOST = "api.binance.com";
inline constexpr std::string_view WS_DEFAULT_TARGET = "/stream";
inline constexpr std::string_view WS_USERDATA_TARGET = "/ws-api/v3";
inline constexpr std::string_view API_DEFAULT_TARGET = "/api/v3/";
inline constexpr int WS_PORT_MAIN = 9443;
inline constexpr int HTTPS_PORT = 443;

inline constexpr std::string_view HTTP_USER_AGENT = "RubyTradeHttpAgent";

namespace Header {
inline constexpr std::string_view APIKEY = "X-MBX-APIKEY";
inline constexpr std::string_view CONTENT_TYPE = "Content-Type";
inline constexpr std::string_view CONTENT_LENGTH = "Content-Length";
inline constexpr std::string_view CONNECTION = "Connection";
} // namespace Header
} // namespace Data

#endif // CONSTANTS_HPP
