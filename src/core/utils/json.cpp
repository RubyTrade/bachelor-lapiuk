#include "json.hpp"
#include "nlohmann/json_fwd.hpp"
#include <optional>
#include <string>

JSONQuery::JSONQuery(const char *json) {
  std::string jsonStr = json;
  if (jsonStr.empty()) {
    m_jsonQuery = nlohmann::json{};
    return;
  }

  if (nlohmann::json::accept(jsonStr)) {
    m_jsonQuery = nlohmann::json::parse(jsonStr);
  } else {
    m_jsonQuery = nlohmann::json{};
  }
}

JSONQuery::JSONQuery(const std::string &json) {
  if (json.empty()) {
    m_jsonQuery = nlohmann::json{};
    return;
  }

  if (nlohmann::json::accept(json)) {
    m_jsonQuery = nlohmann::json::parse(json);
  } else {
    m_jsonQuery = nlohmann::json{};
  }
}

JSONQuery::JSONQuery(const nlohmann::json &json) : m_jsonQuery(json) {}

JSONQuery::JSONQuery(const JSONQuery &json) : m_jsonQuery(json.json()) {}

void JSONQuery::set_value(const std::string &key, const JSONValue &value) {
  std::visit(
      [&](auto &&val) {
        if (m_jsonQuery.contains(key) && m_jsonQuery[key].is_array()) {
          add_to_array(key, value);
          return;
        }

        m_jsonQuery[key] = val;
      },
      value);
}

void JSONQuery::add_to_array(const std::string &key, const JSONValue &value) {
  if (!m_jsonQuery.contains(key)) {
    m_jsonQuery[key] = nlohmann::json::array();
  }

  std::visit(
      [&](auto &&val) {
        auto &array = m_jsonQuery[key];
        if (std::find(array.begin(), array.end(), val) == array.end()) {
          array.push_back(val);
        }
      },
      value);
}

// TODO: optimize get_value with string_view
std::optional<nlohmann::json>
JSONQuery::get_value(const std::string &key) const {
  if (!m_jsonQuery.contains(key))
    return std::nullopt;
  return m_jsonQuery[key];
}

bool JSONQuery::is_key_exists(const std::string &key) const {
  return m_jsonQuery.contains(key);
}

std::map<std::string, JSONValue> JSONQuery::get_map_of_items() const {
  std::map<std::string, JSONValue> result{};

  for (auto &[key, value] : m_jsonQuery.items()) {
    if (value.is_string())
      result[key] = value.get<std::string>();
    else if (value.is_number_unsigned())
      result[key] = value.get<uint64_t>();
    else if (value.is_number_integer())
      result[key] = value.get<int>();
    else if (value.is_boolean())
      result[key] = value.get<bool>();
    else if (value.is_object())
      result[key] = value.get<nlohmann::json>();
    else if (value.is_null())
      result[key] = std::string("null");
    else
      result[key] = value.dump(); // fallback
  }

  return result;
}

/* static */ std::optional<nlohmann::json>
JSONQuery::get_value(const nlohmann::json &json, const std::string &key) {
  if (!json.contains(key))
    return std::nullopt;
  return json[key];
}

/* static */ bool JSONQuery::is_key_exists(const nlohmann::json &json,
                                           const std::string &key) {
  return json.contains(key);
}

void JSONQuery::remove_key(const std::string &key) {
  if (is_key_exists(key))
    m_jsonQuery.erase(key);
}

bool JSONQuery::is_empty() const { return m_jsonQuery.empty(); }
