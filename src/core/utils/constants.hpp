#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string_view>

namespace Data {
inline static constexpr std::string_view BINANCE_READ_APIKEY_ENV =
    "BINANCE_READ_API_KEY";
inline static constexpr std::string_view BINANCE_WRITE_APIKEY_ENV =
    "BINANCE_WRITE_API_KEY";
inline static constexpr std::string_view BINANCE_PK_ENV = "BINANCE_PRIVATE_KEY";

inline constexpr std::string_view WS_HOST = "fstream.binance.com";
inline constexpr std::string_view WS_TRADING_HOST = "ws-fapi.binance.com";
inline constexpr std::string_view WS_DEFAULT_TARGET = "/stream";
inline constexpr std::string_view WS_USERDATA_TARGET = "/ws";
inline constexpr std::string_view WS_TRADING_API_TARGET = "/ws-fapi/v1";

inline constexpr std::string_view API_HOST = "fapi.binance.com";

inline constexpr std::string_view API_DEFAULT_TARGET = "/fapi/v1/";

inline constexpr int WS_PORT_MAIN = 443;
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
