#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "core/utils/log.hpp"

namespace {
struct CoutCapture {
  std::streambuf *old = nullptr;
  std::ostringstream oss;

  void start() {
    old = std::cout.rdbuf(oss.rdbuf());
  }

  std::string stop() {
    std::cout.rdbuf(old);
    return oss.str();
  }
};
} // namespace

TEST(Log, WritesToStdoutWithNewlineByDefault) {
  CoutCapture cap;
  cap.start();
  Log::log("hello");
  const std::string out = cap.stop();
  EXPECT_EQ(out, std::string("hello\n"));
}

TEST(Log, WritesWithoutNewlineWhenDisabled) {
  CoutCapture cap;
  cap.start();
  Log::log("hello", false);
  const std::string out = cap.stop();
  EXPECT_EQ(out, std::string("hello"));
}

TEST(Log, MultipleCallsWithNewlines) {
  CoutCapture cap;
  cap.start();
  Log::log("first");
  Log::log("second");
  const std::string out = cap.stop();
  EXPECT_EQ(out, "first\nsecond\n");
}

TEST(Log, EmptyStringWithNewline) {
  CoutCapture cap;
  cap.start();
  Log::log("");
  const std::string out = cap.stop();
  EXPECT_EQ(out, "\n");
}

TEST(Log, LongMessage) {
  CoutCapture cap;
  std::string longMsg(1000, 'x');
  cap.start();
  Log::log(longMsg);
  const std::string out = cap.stop();
  EXPECT_EQ(out, longMsg + "\n");
}
