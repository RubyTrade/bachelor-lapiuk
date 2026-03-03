#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "core/utils/thread.hpp"

using namespace std::chrono_literals;

TEST(Thread, StartRunsFunction) {
  Thread t;
  std::atomic<bool> ran{false};

  t.start([&] {
    ran = true;
  });

  // Stop joins when joinable.
  t.stop();
  EXPECT_TRUE(ran.load());
}

TEST(Thread, StartWhileRunningDoesNotStartSecondThread) {
  Thread t;
  std::atomic<int> calls{0};

  t.start([&] {
    calls.fetch_add(1);
    std::this_thread::sleep_for(100ms);
  });

  t.start([&] {
    calls.fetch_add(100);
  });

  t.stop();
  EXPECT_EQ(calls.load(), 1);
}

TEST(ThreadUtils, SleepHelpersDoNotThrow) {
  EXPECT_NO_THROW(ThreadUtils::sleepForMs(1));
  EXPECT_NO_THROW(ThreadUtils::sleepForSecs(0));
}

TEST(Thread, StopWhileNotRunningIsSafe) {
  Thread t;
  EXPECT_NO_THROW(t.stop());
  EXPECT_NO_THROW(t.stop());
}

TEST(Thread, MultipleStopsAreSafe) {
  Thread t;
  std::atomic<bool> ran{false};
  
  t.start([&] {
    ran = true;
  });
  
  t.stop();
  t.stop();
  t.stop();
  
  EXPECT_TRUE(ran.load());
}

TEST(Thread, ThreadExecutesLambdaWithCapture) {
  Thread t;
  int value = 0;
  
  t.start([&value] {
    value = 42;
  });
  
  t.stop();
  EXPECT_EQ(value, 42);
}

TEST(Thread, LongRunningTaskCanBeStopped) {
  Thread t;
  std::atomic<bool> running{false};
  std::atomic<bool> stopped{false};
  
  t.start([&] {
    running = true;
    for (int i = 0; i < 1000 && !stopped; ++i) {
      std::this_thread::sleep_for(10ms);
    }
  });
  
  // Wait for thread to start
  while (!running) {
    std::this_thread::sleep_for(1ms);
  }
  
  stopped = true;
  t.stop();
  EXPECT_TRUE(true); // No hang
}
