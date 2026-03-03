#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <set>
#include <thread>
#include <vector>

#include "core/utils/queue.hpp"

using namespace std::chrono_literals;

TEST(Queue, FifoOrderSingleThread) {
  Queue<int> q;
  q.push_message(1);
  q.push_message(2);
  q.push_message(3);

  int out = 0;
  ASSERT_TRUE(q.pop_message(out));
  EXPECT_EQ(out, 1);
  ASSERT_TRUE(q.pop_message(out));
  EXPECT_EQ(out, 2);
  ASSERT_TRUE(q.pop_message(out));
  EXPECT_EQ(out, 3);
}

TEST(Queue, PopBlocksUntilMessageArrives) {
  Queue<int> q;

  std::promise<int> got;
  std::thread t([&] {
    int out = 0;
    if (q.pop_message(out)) {
      got.set_value(out);
    }
  });

  std::this_thread::sleep_for(50ms);
  q.push_message(42);

  auto fut = got.get_future();
  EXPECT_EQ(fut.wait_for(2s), std::future_status::ready);
  EXPECT_EQ(fut.get(), 42);

  t.join();
}

TEST(Queue, StopUnblocksPopAndReturnsFalse) {
  Queue<int> q;

  std::promise<bool> popped;
  std::thread t([&] {
    int out = 0;
    popped.set_value(q.pop_message(out));
  });

  std::this_thread::sleep_for(50ms);
  q.stop_queue();

  auto fut = popped.get_future();
  EXPECT_EQ(fut.wait_for(2s), std::future_status::ready);
  EXPECT_FALSE(fut.get());

  t.join();
}

TEST(Queue, MultiProducerSingleConsumerReceivesAll) {
  Queue<int> q;

  constexpr int producers = 4;
  constexpr int perProducer = 250;
  constexpr int total = producers * perProducer;

  std::atomic<int> consumed{0};
  std::mutex m;
  std::set<int> seen;

  std::thread consumer([&] {
    while (consumed.load() < total) {
      int out = 0;
      if (!q.pop_message(out)) {
        return;
      }
      {
        std::lock_guard<std::mutex> lock(m);
        seen.insert(out);
      }
      consumed.fetch_add(1);
    }
  });

  std::vector<std::thread> prod;
  prod.reserve(producers);
  for (int p = 0; p < producers; ++p) {
    prod.emplace_back([&, p] {
      for (int i = 0; i < perProducer; ++i) {
        q.push_message(p * 100000 + i);
      }
    });
  }

  for (auto &t : prod) {
    t.join();
  }

  // Wait for consumer to drain
  for (int i = 0; i < 200 && consumed.load() < total; ++i) {
    std::this_thread::sleep_for(10ms);
  }

  EXPECT_EQ(consumed.load(), total);
  {
    std::lock_guard<std::mutex> lock(m);
    EXPECT_EQ(static_cast<int>(seen.size()), total);
  }

  consumer.join();
}

TEST(ObservableQueue, CallbackInvokedBeforeEnqueue) {
  ObservableQueue<int> q;

  std::atomic<int> cbCount{0};
  std::atomic<int> last{0};

  q.register_callback([&](const int &v) {
    cbCount.fetch_add(1);
    last.store(v);
  });

  q.push_message(7);
  q.push_message(8);

  EXPECT_EQ(cbCount.load(), 2);
  EXPECT_EQ(last.load(), 8);
}

TEST(Queue, EmptyQueueAfterStop) {
  Queue<int> q;
  q.push_message(1);
  q.push_message(2);
  q.stop_queue();

  int out = 0;
  // After stop, pop should return false
  EXPECT_FALSE(q.pop_message(out));
}

TEST(Queue, PushAfterStopIsIgnored) {
  Queue<int> q;
  q.stop_queue();
  q.push_message(42);

  int out = 0;
  EXPECT_FALSE(q.pop_message(out));
}

TEST(Queue, MultipleStopsAreSafe) {
  Queue<int> q;
  q.stop_queue();
  q.stop_queue();
  q.stop_queue();

  // Should not crash
  EXPECT_TRUE(true);
}

TEST(Queue, LargeNumberOfMessages) {
  Queue<int> q;
  const int count = 10000;

  for (int i = 0; i < count; ++i) {
    q.push_message(std::move(i));
  }

  for (int i = 0; i < count; ++i) {
    int out = 0;
    ASSERT_TRUE(q.pop_message(out));
    EXPECT_EQ(out, i);
  }
}
