#ifndef TIME_HPP
#define TIME_HPP

#include "utils/log.hpp"
#include "utils/thread.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/detail/chrono.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <ratio>
#include <type_traits>
#include <utility>

using work_guard_t =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

namespace chrono = std::chrono;

class AsyncTimer {
public:
  AsyncTimer() : m_is_running(false), m_timer(m_io_context) {
    _run_io_context();
  }

  ~AsyncTimer() {
    _stop_io_context();
    stop();
  }

  // Callback timer will detach the timer Thread
  // and execute callback funcion when it's ready
  //
  // Duration - duration in chrono values,
  // Func - function to run,
  // Args - additional args to pass in func
  template <class Duration, class Func, class... Args>
  void start(const Duration &duration, Func &&callback, Args &&...args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_is_running)
      return;

    if constexpr (!std::is_convertible_v<
                      Duration, std::chrono::duration<long long, std::nano>>)
      return;

    m_end_time = chrono::steady_clock::now() + duration;

    m_is_running = true;

    m_timer.expires_after(duration);

    auto cb =
        std::bind(std::forward<Func>(callback), std::forward<Args>(args)...);

    m_timer.async_wait([this, &duration, cb = std::move(cb)](
                           const boost::system::error_code &ec) {
      if (!ec) {
        cb();
        m_is_running = false;
      }
    });
  }

  // Callback timer will detach the timer Thread
  // and execute callback funcion when it's ready
  // in recurring mode every time as specified in duration
  //
  // Duration - duration in chrono values,
  // Func - function to run,
  // Args - additional args to pass in func
  template <class Duration, class Func, class... Args>
  void start_recurring(const Duration &duration, Func &&callback,
                       Args &&...args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_is_running)
      return;

    if constexpr (!std::is_convertible_v<
                      Duration, std::chrono::duration<long long, std::nano>>)
      return;

    m_end_time = chrono::steady_clock::now() + duration;

    m_is_running = true;

    m_timer.expires_after(duration);

    auto cb =
        std::bind(std::forward<Func>(callback), std::forward<Args>(args)...);

    m_timer.async_wait([this, &duration, cb = std::move(cb)](
                           const boost::system::error_code &ec) {
      if (!ec) {
        cb();
        _schedule_recurring(duration, cb);
      }
    });
  }

  // Condition Variable timer is great when you need
  // to block the thread until the timer runs out
  template <class Duration>
  void start(const Duration &duration, std::condition_variable &cv,
             std::mutex &cv_mtx, bool &cv_flag) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_is_running)
      return;

    if constexpr (!std::is_convertible_v<
                      Duration, std::chrono::duration<long long, std::nano>>)
      return;

    m_end_time = chrono::steady_clock::now() + duration;

    m_is_running = true;

    m_timer.expires_after(duration);

    m_timer.async_wait(
        [this, &cv, &cv_mtx, &cv_flag](const boost::system::error_code &ec) {
          if (!ec) {
            {
              std::lock_guard<std::mutex> lock(cv_mtx);
              cv_flag = true;
            }
            cv.notify_one();

            m_is_running = false;
          }
        });
  }

  void stop() {
    m_timer.cancel();
    m_is_running = false;
  }

  std::atomic_bool isRunning() const { return m_is_running.load(); }

  chrono::milliseconds getEndtime() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_is_running)
      return chrono::milliseconds(0);
    auto now = chrono::steady_clock::now();

    if (now >= m_end_time) {
      return chrono::milliseconds(0);
    }
    return std::chrono::duration_cast<chrono::milliseconds>(m_end_time - now);
  }

private:
  template <class Duration, class Callback>
  void _schedule_recurring(const Duration &duration, Callback &&cb) {
    m_timer.expires_after(duration);

    m_timer.async_wait([this, &duration, cb = std::move(cb)](
                           const boost::system::error_code &ec) {
      if (!ec) {
        cb();
        _schedule_recurring(duration, cb);
      }
    });
  }

  void _run_io_context() {
    m_work_guard.emplace(m_io_context.get_executor());

    m_io_context_thread
        .start([this]() {
          // allow async methods to run
          m_io_context.run();
        })
        .detach();
  }

  void _stop_io_context() {
    m_work_guard.reset();
    m_io_context.stop();
  }

private:
  std::atomic_bool m_is_running;

  // io_context
  boost::asio::io_context m_io_context;
  std::optional<work_guard_t> m_work_guard;
  Thread m_io_context_thread;

  boost::asio::steady_timer m_timer;

  chrono::steady_clock::time_point m_end_time;

  std::mutex m_mutex;
};

#endif // TIME_HPP
