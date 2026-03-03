#ifndef DOTENV_HPP
#define DOTENV_HPP

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <string>

class Env {
public:
  static Env &getInstance() {
    static Env inst;
    return inst;
  }

  void load_dotenv(const std::string &path = ".env") {
    if (!m_is_loaded.load()) {
      std::ifstream file(path);
      std::string line;

      while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
          continue;

        auto pos = line.find("=");
        if (pos == std::string::npos)
          continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        ::setenv(key.c_str(), value.c_str(), 1);
      }

      m_is_loaded = true;
    }
  }

  std::string getenv(const std::string &key) {
    if (key.empty())
      return "";

    const char *val = ::getenv(key.c_str());
    if (!val)
      return "";

    return std::string(val);
  }

  void setenv(const std::string &key, const std::string &value) {
    if (key.empty() || value.empty())
      return;

    ::setenv(key.c_str(), value.c_str(), 1);
  }

private:
  Env() { load_dotenv(); }

  std::atomic_bool m_is_loaded = false;
};

#endif // DOTENV_HPP
