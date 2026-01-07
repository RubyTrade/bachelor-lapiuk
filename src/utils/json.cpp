#include "json.hpp"

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

bool JSONQuery::is_key_exists(const std::string &key) const {
  return m_jsonQuery.contains(key);
}

void JSONQuery::remove_key(const std::string &key) { m_jsonQuery.erase(key); }

bool JSONQuery::is_empty() const { return m_jsonQuery.empty(); }
