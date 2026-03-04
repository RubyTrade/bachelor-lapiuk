#include <gtest/gtest.h>

#include "core/account_manager/account_controller.hpp"
#include "core/parsers/account_rest_api_parser.hpp"
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

// REST API Response Processing Tests

TEST(AccountController, ProcessAccountInfoResponse) {
  AccountController controller;

  std::string accountInfoJson = R"({
    "feeTier": 2,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": false,
    "updateTime": 1700000000000,
    "multiAssetsMargin": false,
    "totalWalletBalance": "10000.0",
    "assets": [
      {
        "asset": "USDT",
        "walletBalance": "10000.0",
        "crossWalletBalance": "9500.0"
      }
    ],
    "positions": [
      {
        "symbol": "BTCUSDT",
        "positionSide": "LONG",
        "positionAmt": "0.1",
        "entryPrice": "50000.0",
        "unrealizedProfit": "100.0",
        "leverage": 10,
        "isolated": false,
        "breakEvenPrice": "50100.0"
      }
    ]
  })";

  JSONQuery json(accountInfoJson);
  AccountRestApi::RestApiMessage msg(
      AccountRestApi::ACCOUNT_API_TYPE::ACCOUNT_INFO, json);
  auto parsed = AccountRestApi::AccountInfoParser::parse(msg);

  controller.processRestApiResponse(parsed);

  // Check permissions
  EXPECT_TRUE(controller.getConfig().canTrade());
  auto permissions = controller.getConfig().getPermissions();
  EXPECT_TRUE(permissions.canTrade);
  EXPECT_TRUE(permissions.canDeposit);
  EXPECT_FALSE(permissions.canWithdraw);
  EXPECT_EQ(permissions.feeTier, 2u);

  // Check balances
  const auto &balances = controller.getBalancesList();
  EXPECT_EQ(balances.size(), 1u);
  EXPECT_TRUE(balances.count("USDT") > 0);

  // Check positions
  const auto &positions = controller.getPositionsList();
  EXPECT_EQ(positions.size(), 1u);

  // Check leverage was set
  EXPECT_EQ(controller.getConfig().getLeverageConfig("BTCUSDT"), 10u);

  // Check margin type was set
  EXPECT_EQ(controller.getConfig().getMarginType("BTCUSDT"),
            MARGIN_TYPE::CROSSED);
}

TEST(AccountController, ProcessAccountInfoWithIsolatedPosition) {
  AccountController controller;

  std::string accountInfoJson = R"({
    "feeTier": 0,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "positions": [
      {
        "symbol": "ETHUSDT",
        "positionSide": "BOTH",
        "positionAmt": "1.0",
        "entryPrice": "3000.0",
        "leverage": 5,
        "isolated": true,
        "unrealizedProfit": "50.0",
        "breakEvenPrice": "3010.0"
      }
    ],
    "assets": []
  })";

  JSONQuery json(accountInfoJson);
  AccountRestApi::RestApiMessage msg(
      AccountRestApi::ACCOUNT_API_TYPE::ACCOUNT_INFO, json);
  auto parsed = AccountRestApi::AccountInfoParser::parse(msg);

  controller.processRestApiResponse(parsed);

  EXPECT_EQ(controller.getConfig().getMarginType("ETHUSDT"),
            MARGIN_TYPE::ISOLATED);
  EXPECT_EQ(controller.getConfig().getLeverageConfig("ETHUSDT"), 5u);
}

TEST(AccountController, ProcessAccountBalanceResponse) {
  AccountController controller;

  std::string balanceJson = R"([
    {
      "accountAlias": "main",
      "asset": "USDT",
      "balance": "5000.0",
      "crossWalletBalance": "4500.0",
      "updateTime": 1700000000000
    },
    {
      "accountAlias": "main",
      "asset": "BTC",
      "balance": "0.5",
      "crossWalletBalance": "0.45",
      "updateTime": 1700000000001
    }
  ])";

  JSONQuery json(balanceJson);
  AccountRestApi::RestApiMessage msg(
      AccountRestApi::ACCOUNT_API_TYPE::ACCOUNT_BALANCE, json);
  auto parsed = AccountRestApi::AccountBalanceParser::parse(msg);

  controller.processRestApiResponse(parsed);

  const auto &balances = controller.getBalancesList();
  EXPECT_EQ(balances.size(), 2u);
  EXPECT_TRUE(balances.count("USDT") > 0);
  EXPECT_TRUE(balances.count("BTC") > 0);
}

TEST(AccountController, ProcessCommissionRateResponse) {
  AccountController controller;

  std::string commissionJson = R"({
    "symbol": "BTCUSDT",
    "makerCommissionRate": "0.0002",
    "takerCommissionRate": "0.0004"
  })";

  JSONQuery json(commissionJson);
  AccountRestApi::RestApiMessage msg(
      AccountRestApi::ACCOUNT_API_TYPE::COMMISSION_RATE, json);
  auto parsed = AccountRestApi::CommissionRateParser::parse(msg);

  controller.processRestApiResponse(parsed);

  auto [maker, taker] = controller.getConfig().getCommissionRate("BTCUSDT");
  EXPECT_EQ(maker.to_string(), "0.0002");
  EXPECT_EQ(taker.to_string(), "0.0004");
}

TEST(AccountController, ProcessErrorParseFromRestApi) {
  AccountController controller;

  ErrorParse error;
  error.parse_error = "Test REST API error";

  AccountRestApi::ParsedAccountRestApi parsedError = error;
  
  // Should not crash
  controller.processRestApiResponse(parsedError);
  
  // Controller should still be functional
  EXPECT_TRUE(controller.getBalancesList().empty());
}

TEST(AccountController, ProcessMultipleRestApiResponses) {
  AccountController controller;

  // First: Account Info
  std::string accountInfoJson = R"({
    "feeTier": 1,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "multiAssetsMargin": false,
    "positions": [],
    "assets": []
  })";

  JSONQuery json1(accountInfoJson);
  AccountRestApi::RestApiMessage msg1(
      AccountRestApi::ACCOUNT_API_TYPE::ACCOUNT_INFO, json1);
  auto parsed1 = AccountRestApi::AccountInfoParser::parse(msg1);
  controller.processRestApiResponse(parsed1);

  EXPECT_TRUE(controller.getConfig().canTrade());

  // Second: Commission Rate
  std::string commissionJson = R"({
    "symbol": "ETHUSDT",
    "makerCommissionRate": "0.00015",
    "takerCommissionRate": "0.00035"
  })";

  JSONQuery json2(commissionJson);
  AccountRestApi::RestApiMessage msg2(
      AccountRestApi::ACCOUNT_API_TYPE::COMMISSION_RATE, json2);
  auto parsed2 = AccountRestApi::CommissionRateParser::parse(msg2);
  controller.processRestApiResponse(parsed2);

  auto [maker, taker] = controller.getConfig().getCommissionRate("ETHUSDT");
  EXPECT_EQ(maker.to_string(), "0.00015");

  // Third: Balance
  std::string balanceJson = R"([
    {
      "asset": "USDT",
      "balance": "1000.0",
      "crossWalletBalance": "900.0"
    }
  ])";

  JSONQuery json3(balanceJson);
  AccountRestApi::RestApiMessage msg3(
      AccountRestApi::ACCOUNT_API_TYPE::ACCOUNT_BALANCE, json3);
  auto parsed3 = AccountRestApi::AccountBalanceParser::parse(msg3);
  controller.processRestApiResponse(parsed3);

  EXPECT_EQ(controller.getBalancesList().size(), 1u);
}

TEST(AccountController, GetConfigAccess) {
  AccountController controller;

  // Test const access
  const AccountController &constController = controller;
  const AccountConfig &constConfig = constController.getConfig();
  EXPECT_FALSE(constConfig.canTrade());

  // Test non-const access
  AccountConfig &config = controller.getConfig();
  AccountPermissions permissions;
  permissions.canTrade = true;
  config.updatePermissions(permissions);

  EXPECT_TRUE(controller.getConfig().canTrade());
}

TEST(AccountController, AccountInfoWithMultiplePositions) {
  AccountController controller;

  std::string accountInfoJson = R"({
    "feeTier": 0,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "positions": [
      {
        "symbol": "BTCUSDT",
        "positionSide": "LONG",
        "positionAmt": "0.1",
        "entryPrice": "50000.0",
        "leverage": 10,
        "isolated": false,
        "unrealizedProfit": "100.0",
        "breakEvenPrice": "50100.0"
      },
      {
        "symbol": "ETHUSDT",
        "positionSide": "SHORT",
        "positionAmt": "-1.0",
        "entryPrice": "3000.0",
        "leverage": 5,
        "isolated": true,
        "unrealizedProfit": "-50.0",
        "breakEvenPrice": "2990.0"
      },
      {
        "symbol": "BNBUSDT",
        "positionSide": "BOTH",
        "positionAmt": "10.0",
        "entryPrice": "300.0",
        "leverage": 3,
        "isolated": false,
        "unrealizedProfit": "20.0",
        "breakEvenPrice": "302.0"
      }
    ],
    "assets": []
  })";

  JSONQuery json(accountInfoJson);
  AccountRestApi::RestApiMessage msg(
      AccountRestApi::ACCOUNT_API_TYPE::ACCOUNT_INFO, json);
  auto parsed = AccountRestApi::AccountInfoParser::parse(msg);

  controller.processRestApiResponse(parsed);

  EXPECT_EQ(controller.getPositionsList().size(), 3u);
  
  // Check leverage for each symbol
  EXPECT_EQ(controller.getConfig().getLeverageConfig("BTCUSDT"), 10u);
  EXPECT_EQ(controller.getConfig().getLeverageConfig("ETHUSDT"), 5u);
  EXPECT_EQ(controller.getConfig().getLeverageConfig("BNBUSDT"), 3u);
  
  // Check margin types
  EXPECT_EQ(controller.getConfig().getMarginType("BTCUSDT"),
            MARGIN_TYPE::CROSSED);
  EXPECT_EQ(controller.getConfig().getMarginType("ETHUSDT"),
            MARGIN_TYPE::ISOLATED);
  EXPECT_EQ(controller.getConfig().getMarginType("BNBUSDT"),
            MARGIN_TYPE::CROSSED);
}

TEST(AccountController, AccountInfoClosesZeroPositions) {
  AccountController controller;

  // First add a position via REST API
  std::string accountInfoJson1 = R"({
    "feeTier": 0,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "positions": [
      {
        "symbol": "BTCUSDT",
        "positionSide": "BOTH",
        "positionAmt": "0.1",
        "entryPrice": "50000.0",
        "leverage": 10,
        "isolated": false,
        "unrealizedProfit": "0.0",
        "breakEvenPrice": "50000.0"
      }
    ],
    "assets": []
  })";

  JSONQuery json1(accountInfoJson1);
  AccountRestApi::RestApiMessage msg1(
      AccountRestApi::ACCOUNT_API_TYPE::ACCOUNT_INFO, json1);
  auto parsed1 = AccountRestApi::AccountInfoParser::parse(msg1);
  controller.processRestApiResponse(parsed1);

  EXPECT_EQ(controller.getPositionsList().size(), 1u);

  // Now send account info with zero position
  std::string accountInfoJson2 = R"({
    "feeTier": 0,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "positions": [
      {
        "symbol": "BTCUSDT",
        "positionSide": "BOTH",
        "positionAmt": "0.0",
        "entryPrice": "0.0",
        "leverage": 10,
        "isolated": false,
        "unrealizedProfit": "0.0",
        "breakEvenPrice": "0.0"
      }
    ],
    "assets": []
  })";

  JSONQuery json2(accountInfoJson2);
  AccountRestApi::RestApiMessage msg2(
      AccountRestApi::ACCOUNT_API_TYPE::ACCOUNT_INFO, json2);
  auto parsed2 = AccountRestApi::AccountInfoParser::parse(msg2);
  controller.processRestApiResponse(parsed2);

  EXPECT_EQ(controller.getPositionsList().size(), 0u);
}

