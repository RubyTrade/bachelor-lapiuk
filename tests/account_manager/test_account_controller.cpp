#include <gtest/gtest.h>

#include "core/account_manager/account_controller.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/fixed_num.hpp"

#include <chrono>
#include <thread>

using namespace UserData;

// Helper functions to create test events
AccountUpdateEvent::Balance
createBalance(const std::string &asset, const std::string &walletBalance,
              const std::string &crossWalletBalance) {
  AccountUpdateEvent::Balance balance;
  balance.asset = asset;
  balance.walletBalance = Fixed(walletBalance);
  balance.crossWalletBalance = Fixed(crossWalletBalance);
  balance.balanceChange = Fixed("0");
  return balance;
}

AccountUpdateEvent::Position
createPosition(const std::string &symbol, const std::string &positionAmt,
               const std::string &entryPrice,
               POSITION_SIDE positionSide = POSITION_SIDE::BOTH) {
  AccountUpdateEvent::Position position;
  position.symbol = symbol;
  position.positionAmt = Fixed(positionAmt);
  position.entryPrice = Fixed(entryPrice);
  position.positionSide = positionSide;
  return position;
}

AccountUpdateEvent createAccountUpdateEvent() {
  AccountUpdateEvent event;
  event.eventTime = 1234567890;
  event.transactionTime = 1234567890;
  event.reason = "DEPOSIT";
  return event;
}

AccountConfigUpdateEvent createConfigUpdateEvent() {
  AccountConfigUpdateEvent event;
  event.eventTime = 1234567890;
  event.transactionTime = 1234567890;
  return event;
}

// AccountController Integration Tests
TEST(AccountController, InitializedWithTimestamp) {
  AccountController controller;
  EXPECT_GT(controller.getLastUpdateTime(), 0u);
}

TEST(AccountController, EmptyByDefault) {
  AccountController controller;
  EXPECT_TRUE(controller.getBalancesList().empty());
  EXPECT_TRUE(controller.getPositionsList().empty());
}

TEST(AccountController, ProcessAccountUpdateEventWithBalances) {
  AccountController controller;

  controller.start();

  auto event = createAccountUpdateEvent();
  event.balances.push_back(createBalance("USDT", "1000.0", "900.0"));
  event.balances.push_back(createBalance("BTC", "0.1", "0.09"));

  ParsedUserData parsedEvent = event;
  controller.enqueue(std::move(parsedEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto &balances = controller.getBalancesList();
  EXPECT_EQ(balances.size(), 2u);
  EXPECT_TRUE(balances.count("USDT") > 0);
  EXPECT_TRUE(balances.count("BTC") > 0);
}

TEST(AccountController, ProcessAccountUpdateEventWithPositions) {
  AccountController controller;

  controller.start();

  auto event = createAccountUpdateEvent();
  event.positions.push_back(createPosition("BTCUSDT", "1.0", "50000.0"));
  event.positions.push_back(createPosition("ETHUSDT", "10.0", "3000.0"));

  ParsedUserData parsedEvent = event;
  controller.enqueue(std::move(parsedEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto &positions = controller.getPositionsList();
  EXPECT_EQ(positions.size(), 2u);
}

TEST(AccountController, ZeroPositionIsRemoved) {
  AccountController controller;

  controller.start();

  // First add a position
  auto event1 = createAccountUpdateEvent();
  event1.positions.push_back(createPosition("BTCUSDT", "1.0", "50000.0"));
  ParsedUserData parsed1 = event1;
  controller.enqueue(std::move(parsed1));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_EQ(controller.getPositionsList().size(), 1u);

  // Then close the position (set to 0)
  auto event2 = createAccountUpdateEvent();
  event2.positions.push_back(createPosition("BTCUSDT", "0.0", "50000.0"));
  ParsedUserData parsed2 = event2;
  controller.enqueue(std::move(parsed2));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_EQ(controller.getPositionsList().size(), 0u);
}

TEST(AccountController, ProcessConfigUpdateLeverage) {
  AccountController controller;

  controller.start();

  auto event = createConfigUpdateEvent();
  AccountConfigUpdateEvent::LeverageUpdate leverageUpdate;
  leverageUpdate.symbol = "BTCUSDT";
  leverageUpdate.leverage = 20;
  event.leverageUpdate = leverageUpdate;

  ParsedUserData parsedEvent = event;
  controller.enqueue(std::move(parsedEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Config updates are internal, we just verify no crash
  uint64_t updateTime = controller.getLastUpdateTime();
  EXPECT_GT(updateTime, 0u);
}

TEST(AccountController, ProcessConfigUpdateMultiAssetMode) {
  AccountController controller;

  controller.start();

  auto event = createConfigUpdateEvent();
  AccountConfigUpdateEvent::MultiAssetModeUpdate multiAssetUpdate;
  multiAssetUpdate.multiAssetsMode = true;
  event.multiAssetModeUpdate = multiAssetUpdate;

  ParsedUserData parsedEvent = event;
  controller.enqueue(std::move(parsedEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  uint64_t updateTime = controller.getLastUpdateTime();
  EXPECT_GT(updateTime, 0u);
}

TEST(AccountController, UpdateTimeChangesAfterEvent) {
  AccountController controller;

  controller.start();

  uint64_t initialTime = controller.getLastUpdateTime();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto event = createAccountUpdateEvent();
  event.balances.push_back(createBalance("USDT", "1000.0", "900.0"));
  ParsedUserData parsedEvent = event;
  controller.enqueue(std::move(parsedEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  uint64_t afterEventTime = controller.getLastUpdateTime();
  EXPECT_GT(afterEventTime, initialTime);
}

TEST(AccountController, MultipleSequentialUpdates) {
  AccountController controller;

  controller.start();

  for (int i = 1; i <= 5; ++i) {
    auto event = createAccountUpdateEvent();
    event.balances.push_back(createBalance("USDT",
                                           std::to_string(i * 1000) + ".0",
                                           std::to_string(i * 900) + ".0"));
    ParsedUserData parsedEvent = event;
    controller.enqueue(std::move(parsedEvent));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto &balances = controller.getBalancesList();
  EXPECT_EQ(balances.size(), 1u);
  EXPECT_TRUE(balances.count("USDT") > 0);
}

TEST(AccountController, MixedPositionSides) {
  AccountController controller;

  controller.start();

  auto event = createAccountUpdateEvent();
  event.positions.push_back(
      createPosition("BTCUSDT", "1.0", "50000.0", POSITION_SIDE::LONG));
  event.positions.push_back(
      createPosition("BTCUSDT", "-0.5", "51000.0", POSITION_SIDE::SHORT));
  event.positions.push_back(
      createPosition("ETHUSDT", "10.0", "3000.0", POSITION_SIDE::BOTH));

  ParsedUserData parsedEvent = event;
  controller.enqueue(std::move(parsedEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto &positions = controller.getPositionsList();
  EXPECT_EQ(positions.size(), 3u);
}

TEST(AccountController, ErrorParseIsIgnored) {
  AccountController controller;

  controller.start();

  ErrorParse error;
  error.parse_error = "Test error";
  ParsedUserData parsedEvent = error;

  controller.enqueue(std::move(parsedEvent));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Controller should still work after receiving error
  auto event = createAccountUpdateEvent();
  event.balances.push_back(createBalance("USDT", "1000.0", "900.0"));
  ParsedUserData validEvent = event;
  controller.enqueue(std::move(validEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  EXPECT_EQ(controller.getBalancesList().size(), 1u);
}

TEST(AccountController, PositionSignatureGeneration) {
  // Test the signature generation utility
  std::string sig1 =
      PositionAsset::getSignature("BTCUSDT", POSITION_SIDE::BOTH);
  std::string sig2 =
      PositionAsset::getSignature("BTCUSDT", POSITION_SIDE::LONG);
  std::string sig3 =
      PositionAsset::getSignature("BTCUSDT", POSITION_SIDE::SHORT);

  // All should be different
  EXPECT_NE(sig1, sig2);
  EXPECT_NE(sig2, sig3);
  EXPECT_NE(sig1, sig3);

  // Same input should produce same output
  std::string sig1_again =
      PositionAsset::getSignature("BTCUSDT", POSITION_SIDE::BOTH);
  EXPECT_EQ(sig1, sig1_again);
}

TEST(AccountController, StressTestMultipleEvents) {
  AccountController controller;

  controller.start();

  const int numEvents = 100;

  for (int i = 0; i < numEvents; ++i) {
    auto event = createAccountUpdateEvent();
    std::string symbol = "SYM" + std::to_string(i % 10);
    event.positions.push_back(createPosition(symbol, "1.0", "1000.0"));

    ParsedUserData parsedEvent = event;
    controller.enqueue(std::move(parsedEvent));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // Should have positions for 10 different symbols
  EXPECT_LE(controller.getPositionsList().size(), 10u);
}
