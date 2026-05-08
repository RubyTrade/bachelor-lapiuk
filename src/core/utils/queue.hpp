#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

template <class Type> class Queue {
public:
  void push_message(Type &&msg) {
    if (!m_is_active)
      return;

    {
      std::lock_guard<std::mutex> lock(m_mtxQueue);
      m_queue.push(std::move(msg));
    }

    cv.notify_one();
  }

  bool pop_message(Type &out_msg) {
    std::unique_lock<std::mutex> lock(m_mtxQueue);
    cv.wait(lock, [&] { return !m_is_active || !m_queue.empty(); });

    if (!m_is_active || m_queue.empty())
      return false;

    out_msg = std::move(m_queue.front());
    m_queue.pop();

    return true;
  }

  bool is_empty() {
    std::lock_guard<std::mutex> lock(m_mtxQueue);
    return m_queue.empty();
  }

  size_t get_size() {
    std::lock_guard<std::mutex> lock(m_mtxQueue);
    return m_queue.size();
  }

  void stop_queue() {
    m_is_active = false;
    cv.notify_all();
  }

  void start_queue() {
    m_is_active = true;
    cv.notify_all();
  }

  Queue() { start_queue(); };
  Queue(Queue &&) = delete;
  Queue(const Queue &) = delete;
  Queue &operator=(Queue &&) = delete;
  Queue &operator=(const Queue &) = delete;
  ~Queue() { stop_queue(); };

private:
  std::atomic_bool m_is_active;

  std::queue<Type> m_queue;
  std::mutex m_mtxQueue;
  std::condition_variable cv;
};

template <class QueueType> class ObservableQueue {
private:
  Queue<QueueType> m_queue;
  std::function<void(const QueueType &)> m_callback;

public:
  void push_message(QueueType &&msg) {
    if (m_callback)
      m_callback(msg);
    m_queue.push_message(std::move(msg));
  }

  bool pop_message(QueueType &out_msg) { return m_queue.pop_message(out_msg); }

  void register_callback(std::function<void(const QueueType &)> cb) {
    m_callback = cb;
  }
};

#endif // QUEUE_HPP
