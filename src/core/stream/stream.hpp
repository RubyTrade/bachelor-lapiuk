#ifndef STREAM_HPP
#define STREAM_HPP

#include "core/net/net.hpp"
#include "core/utils/json.hpp"
#include "core/utils/queue.hpp"

// TODO: change streams query id's to be UUID type

template <typename Enum> struct EnumStringPair {
  Enum type;
  std::string_view str;
};

template <typename Enum, size_t N>
inline static Enum
str_to_type(const std::array<EnumStringPair<Enum>, N> &type_array,
            const std::string &method) {
  for (auto &&e : type_array) {
    if (e.str == method)
      return e.type;
  }

  return type_array[0].type;
}

template <typename Enum, size_t N>
inline static std::string
type_to_str(const std::array<EnumStringPair<Enum>, N> &type_array,
            const Enum &method) {
  for (auto &&e : type_array) {
    if (e.type == method)
      return std::string(e.str);
  }

  // default fallback
  return "invalid.method";
}

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
