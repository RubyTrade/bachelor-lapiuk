#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include "core/utils/time.hpp"

using namespace std::chrono_literals;

TEST(AsyncTimer, StartExecutesCallbackOnce) {
  AsyncTimer timer;

  std::promise<void> fired;
  timer.start(100ms, [&] { fired.set_value(); });

  auto fut = fired.get_future();
  EXPECT_EQ(fut.wait_for(2s), std::future_status::ready);

  // Give the timer a moment to flip the flag.
  for (int i = 0; i < 100 && timer.isRunning(); ++i) {
    std::this_thread::sleep_for(5ms);
  }
  EXPECT_FALSE(timer.isRunning());
}

TEST(AsyncTimer, StartWhileRunningIsIgnored) {
  AsyncTimer timer;

  std::atomic<int> first{0};
  std::atomic<int> second{0};

  timer.start(200ms, [&] { first.fetch_add(1); });
  timer.start(10ms, [&] { second.fetch_add(1); });

  std::this_thread::sleep_for(50ms);
  EXPECT_EQ(second.load(), 0);

  std::this_thread::sleep_for(300ms);
  EXPECT_EQ(first.load(), 1);
}

TEST(AsyncTimer, StopCancelsCallback) {
  AsyncTimer timer;

  std::promise<void> fired;
  timer.start(500ms, [&] { fired.set_value(); });
  timer.stop();

  auto fut = fired.get_future();
  EXPECT_EQ(fut.wait_for(200ms), std::future_status::timeout);
  EXPECT_FALSE(timer.isRunning());
}

TEST(AsyncTimer, ConditionVariableStartNotifies) {
  AsyncTimer timer;

  std::condition_variable cv;
  std::mutex m;
  bool flag = false;

  timer.start(100ms, cv, m, flag);

  std::unique_lock<std::mutex> lock(m);
  const bool ok = cv.wait_for(lock, 2s, [&] { return flag; });
  EXPECT_TRUE(ok);
  EXPECT_TRUE(flag);
}

TEST(AsyncTimer, RecurringFiresMultipleTimesAndStopHalts) {
  AsyncTimer timer;

  std::mutex m;
  std::condition_variable cv;
  int count = 0;

  timer.start_recurring(50ms, [&] {
    std::lock_guard<std::mutex> lock(m);
    ++count;
    cv.notify_one();
  });

  {
    std::unique_lock<std::mutex> lock(m);
    const bool ok = cv.wait_for(lock, 2s, [&] { return count >= 3; });
    EXPECT_TRUE(ok);
  }

  timer.stop();
  const int stoppedAt = count;

  std::this_thread::sleep_for(200ms);
  EXPECT_EQ(count, stoppedAt);
}

TEST(AsyncTimer, GetEndtimeReturnsNonNegativeAndEventuallyZero) {
  AsyncTimer timer;
  EXPECT_EQ(timer.getEndtime().count(), 0);

  timer.start(100ms, [] {});
  EXPECT_GT(timer.getEndtime().count(), 0);

  std::this_thread::sleep_for(200ms);
  EXPECT_EQ(timer.getEndtime().count(), 0);
}

TEST(AsyncTimer, MultipleRecurringCallbacks) {
  AsyncTimer timer;
  std::atomic<int> count{0};

  timer.start_recurring(30ms, [&count] { count.fetch_add(1); });

  std::this_thread::sleep_for(150ms);
  timer.stop();

  EXPECT_GE(count.load(), 3);
}

TEST(AsyncTimer, StopBeforeStartIsNoop) {
  AsyncTimer timer;
  EXPECT_NO_THROW(timer.stop());
  EXPECT_FALSE(timer.isRunning());
}

TEST(AsyncTimer, VeryShortDurationTimer) {
  AsyncTimer timer;
  auto fired = std::make_shared<std::promise<void>>();
  auto fut = fired->get_future();

  timer.start(1ms, [fired] { fired->set_value(); });

  EXPECT_EQ(fut.wait_for(1s), std::future_status::ready);

  timer.start(200ms, [] {});

  auto end1 = timer.getEndtime();
  EXPECT_GE(end1.count(), 0);
  EXPECT_LE(end1.count(), 200);

  std::this_thread::sleep_for(250ms);
  auto end2 = timer.getEndtime();
  EXPECT_EQ(end2.count(), 0);
}
