#include <gtest/gtest.h>

#include <string>

#include "core/utils/dotenv.hpp"

TEST(Env, SetenvThenGetenv) {
  auto &env = Env::getInstance();

  const std::string key = "TRADINGBOT_TEST_KEY";
  const std::string val = "VALUE123";

  env.setenv(key, val);
  EXPECT_EQ(env.getenv(key), val);
}

TEST(Env, GetenvMissingReturnsEmptyString) {
  auto &env = Env::getInstance();

  // Unlikely to exist.
  const std::string missing = "TRADINGBOT__THIS_SHOULD_NOT_EXIST__";
  const std::string v = env.getenv(missing);
  EXPECT_TRUE(v.empty());
}

TEST(Env, GetenvEmptyKeyReturnsEmptyString) {
  auto &env = Env::getInstance();
  EXPECT_TRUE(env.getenv("").empty());
}

TEST(Env, OverwriteEnvironmentVariable) {
  auto &env = Env::getInstance();
  
  const std::string key = "TRADINGBOT_OVERWRITE_TEST";
  env.setenv(key, "first");
  EXPECT_EQ(env.getenv(key), "first");
  
  env.setenv(key, "second");
  EXPECT_EQ(env.getenv(key), "second");
}

TEST(Env, MultipleVariables) {
  auto &env = Env::getInstance();
  
  env.setenv("VAR1", "value1");
  env.setenv("VAR2", "value2");
  env.setenv("VAR3", "value3");
  
  EXPECT_EQ(env.getenv("VAR1"), "value1");
  EXPECT_EQ(env.getenv("VAR2"), "value2");
  EXPECT_EQ(env.getenv("VAR3"), "value3");
}

TEST(Env, EmptyValueIsValid) {
  auto &env = Env::getInstance();
  
  const std::string key = "EMPTY_VALUE_TEST";
  env.setenv(key, "");
  
  // Empty string is different from non-existent
  std::string val = env.getenv(key);
  EXPECT_TRUE(val.empty());
}
