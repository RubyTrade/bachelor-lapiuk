#ifndef LOG_HPP
#define LOG_HPP

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

class Log {
public:
  // TODO: add log levels
  // TODO: add tags
  /// If TRADINGBOT_LOG_FILE is set (e.g. in .env), mirror logs to that path.
  static void set_log_file_from_env(const char *env_var = "TRADINGBOT_LOG_FILE") {
    if (!env_var)
      return;
    const char *p = std::getenv(env_var);
    set_log_file(p ? std::string(p) : std::string{});
  }

  /// Append all log_info / log_debug / log_err lines to this path (empty path closes file).
  static void set_log_file(const std::string &path) {
    std::lock_guard<std::mutex> lock(s_ioMutex);
    s_file.reset();
    if (path.empty())
      return;
    auto out = std::make_unique<std::ofstream>(path, std::ios::app);
    if (out->good())
      s_file = std::move(out);
  }

  static void log_debug(const std::string &msg, bool withEndl = true) {
#ifdef DEBUG_BUILD
    _log(msg, withEndl);
#endif // DEBUG_BUILD
  }

  static void log_info(const std::string &msg, bool withEndl = true) {
    _log(msg, withEndl);
  }

  static void log_err(const std::string &msg, bool withEndl = true) {
    std::lock_guard<std::mutex> lock(s_ioMutex);

    std::cerr << msg;
    if (withEndl)
      std::cerr << "\n";
    _write_file_unlocked(msg, withEndl);
  }

private:
  static void _write_file_unlocked(const std::string &msg, bool withEndl) {
    if (!s_file || !s_file->good())
      return;
    *s_file << msg;
    if (withEndl)
      *s_file << '\n';
    s_file->flush();
  }

  static void _log(const std::string &msg, bool withEndl = true) {
    std::lock_guard<std::mutex> lock(s_ioMutex);

    std::cout << msg;
    if (withEndl)
      std::cout << "\n";
    _write_file_unlocked(msg, withEndl);
  }

private:
  inline static std::mutex s_ioMutex;
  inline static std::unique_ptr<std::ofstream> s_file;
};

#endif // LOG_HPP
