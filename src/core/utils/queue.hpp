#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <utility>

template <class Type> class Queue {
public:
  void push_message(std::string &&msg) {
    std::lock_guard<std::mutex> lock(m_mtxQueue);
    m_queue.push(std::move(msg));

    cv.notify_one();
  }

  bool pop_message(std::string &out_msg) {
    std::unique_lock<std::mutex> lock(m_mtxQueue);
    cv.wait(lock, [&] { return !m_queue.empty(); });

    out_msg = std::move(m_queue.front());
    m_queue.pop();
    return !out_msg.empty();
  }

  bool is_empty() {
    std::lock_guard<std::mutex> lock(m_mtxQueue);
    return m_queue.empty();
  }

  size_t get_size() {
    std::lock_guard<std::mutex> lock(m_mtxQueue);
    return m_queue.size();
  }

  Queue() {};
  Queue(Queue &&) = delete;
  Queue(const Queue &) = delete;
  Queue &operator=(Queue &&) = delete;
  Queue &operator=(const Queue &) = delete;
  ~Queue() {};

private:
  std::queue<Type> m_queue;
  std::mutex m_mtxQueue;
  std::condition_variable cv;
};

#endif // QUEUE_HPP
