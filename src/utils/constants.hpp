#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string_view>

namespace Data {

inline constexpr std::string_view WS_HOST = "stream.binance.com";
inline constexpr std::string_view DEFAULT_TARGET = "/stream";
inline constexpr int WS_PORT_MAIN = 9443;
inline constexpr int WS_PORT_BACKUP = 443;

} // namespace Data

#endif // CONSTANTS_HPP
