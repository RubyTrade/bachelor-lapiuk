#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <variant>

#include <nlohmann/json.hpp>

// One of this values might be in JSONValue
using JSONValue = std::variant<std::string, bool, int, nlohmann::json>;

class JSONQuery {
public:
  std::string str() const { return m_jsonQuery.dump(); }
  nlohmann::json json() const { return m_jsonQuery; }

  void set_value(const std::string &key, const JSONValue &value);
  void add_to_array(const std::string &key, const JSONValue &value);
  void remove_key(const std::string &key);

  bool is_key_exists(const std::string &key) const;
  bool is_empty() const;

private:
  nlohmann::json m_jsonQuery = {};
};

#endif // JSON_HPP
