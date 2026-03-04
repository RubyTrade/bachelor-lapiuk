#ifndef JSON_HPP
#define JSON_HPP

#include "nlohmann/json_fwd.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

// One of this values might be in JSONValue
using JSONValue =
    std::variant<std::string, bool, int, uint64_t, nlohmann::json>;

class JSONQuery {
public:
  JSONQuery() = default;

  JSONQuery(const char *json);
  JSONQuery(const std::string &json);
  JSONQuery(const nlohmann::json &json);
  JSONQuery(const JSONQuery &json);

  std::string str() const { return m_jsonQuery.dump(); }
  nlohmann::json json() const { return m_jsonQuery; }

  void set_value(const std::string &key, const JSONValue &value);
  void add_to_array(const std::string &key, const JSONValue &value);
  void remove_key(const std::string &key);

  std::optional<nlohmann::json> get_value(const std::string &key) const;
  bool is_key_exists(const std::string &key) const;
  bool is_empty() const;
  bool is_array() const;
  std::optional<nlohmann::json> get_array() const;

  std::map<std::string, JSONValue> get_map_of_items() const;

  static std::optional<nlohmann::json> get_value(const nlohmann::json &json,
                                                 const std::string &key);
  static bool is_key_exists(const nlohmann::json &json, const std::string &key);

private:
  nlohmann::json m_jsonQuery = {};
};

#endif // JSON_HPP
