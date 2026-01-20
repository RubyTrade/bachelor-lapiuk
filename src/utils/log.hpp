#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <mutex>

class Log {
public:
  // TODO: add log levels
  // TODO: maybe add logging to files
  // TODO: add tags
  static void log(const std::string &msg, bool withEndl = true) {
    std::lock_guard<std::mutex> lock(s_ioMutex);
    std::cout << msg;
    if (withEndl)
      std::cout << "\n";
  }

  static void log_err(const std::string &msg, bool withEndl = true) {
    std::lock_guard<std::mutex> lock(s_ioMutex);

    std::cerr << msg;
    if (withEndl)
      std::cerr << "\n";
  }

private:
  inline static std::mutex s_ioMutex;
};

#endif // LOG_HPP
