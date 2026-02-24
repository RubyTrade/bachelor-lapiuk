#ifndef THREAD_HPP
#define THREAD_HPP

#include <atomic>
#include <chrono>
#include <thread>
#include <utility>

class Thread {
private:
  std::thread m_thread;
  std::atomic_bool m_isRunning;

public:
  Thread() : m_isRunning(false) {}

  template <class Func, class... Args> // Func - function to run,
                                       // Args - additional args to pass in func
  Thread &start(Func &&func, Args &&...args) {
    if (!m_isRunning) {
      m_isRunning = true;
      m_thread =
          std::thread(std::forward<Func>(func), std::forward<Args>(args)...);
    }

    return *this;
  }

  Thread &detach() {
    if (m_isRunning) {
      m_thread.detach();
    }

    return *this;
  }

  void stop() {
    if (m_isRunning) {
      m_isRunning = false;
      if (m_thread.joinable())
        m_thread.join();
    }
  }

  ~Thread() { stop(); }
};

// TODO: deprecate this ThreadUtils?
namespace ThreadUtils {

inline void sleepForSecs(int secs) {
  std::this_thread::sleep_for(std::chrono::seconds(secs));
}

inline void sleepForMs(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace ThreadUtils

#endif // THREAD_HPP
