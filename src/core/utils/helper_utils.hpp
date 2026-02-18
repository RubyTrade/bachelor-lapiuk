#ifndef HELPER_UTILS_HPP
#define HELPER_UTILS_HPP

#include <array>
#include <string>

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
  return "invalid";
}

#endif // HELPER_UTILS_HPP
