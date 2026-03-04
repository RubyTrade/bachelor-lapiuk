#include <gtest/gtest.h>

#include "core/controllers/account_rest_api_utils.hpp"
#include "core/parsers/account_rest_api_parser.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/utils/fixed_num.hpp"

#include <string>

namespace {

using namespace AccountRestApi;

RestApiMessage makeMessage(ACCOUNT_API_TYPE type, const std::string &jsonData) {
  JSONQuery json(jsonData);
  return RestApiMessage(type, json);
}

template <typename... Ts>
void expectErrorParseMessage(const std::variant<Ts...> &result,
                             const std::string &expected) {
  ASSERT_TRUE(std::holds_alternative<ErrorParse>(result));
  auto error = std::get<ErrorParse>(result);
  EXPECT_NE(error.parse_error.find(expected), std::string::npos)
      << "Expected error to contain: " << expected
      << ", but got: " << error.parse_error;
}

} // namespace

// ============================================================================
// AccountInfoParser Tests
// ============================================================================

TEST(AccountRestApiParser, ParseAccountInfoHappyPath) {
  const std::string data = R"({
    "feeTier": 2,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "updateTime": 1700000000000,
    "multiAssetsMargin": false,
    "totalInitialMargin": "1000.50",
    "totalMaintMargin": "500.25",
    "totalWalletBalance": "10000.0",
    "totalUnrealizedProfit": "100.5",
    "totalMarginBalance": "10100.5",
    "totalPositionInitialMargin": "800.0",
    "totalOpenOrderInitialMargin": "200.5",
    "totalCrossWalletBalance": "9500.0",
    "totalCrossUnPnl": "100.5",
    "availableBalance": "8599.5",
    "maxWithdrawAmount": "8599.5",
    "assets": [],
    "positions": []
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_INFO, data);
  auto result = AccountInfoParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<AccountInfoResponse>(result));
  const auto &info = std::get<AccountInfoResponse>(result);

  EXPECT_EQ(info.feeTier, 2u);
  EXPECT_EQ(info.canTrade, true);
  EXPECT_EQ(info.canDeposit, true);
  EXPECT_EQ(info.canWithdraw, true);
  EXPECT_EQ(info.updateTime, 1700000000000u);
  EXPECT_EQ(info.multiAssetsMargin, false);
  EXPECT_EQ(info.totalInitialMargin.to_string(), "1000.50");
  EXPECT_EQ(info.totalMaintMargin.to_string(), "500.25");
  EXPECT_EQ(info.totalWalletBalance.to_string(), "10000.0");
  EXPECT_EQ(info.totalUnrealizedProfit.to_string(), "100.5");
  EXPECT_EQ(info.availableBalance.to_string(), "8599.5");
}

TEST(AccountRestApiParser, ParseAccountInfoWithAssets) {
  const std::string data = R"({
    "feeTier": 0,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "updateTime": 1700000000000,
    "assets": [
      {
        "asset": "USDT",
        "walletBalance": "5000.0",
        "unrealizedProfit": "50.0",
        "marginBalance": "5050.0",
        "maintMargin": "100.0",
        "initialMargin": "200.0",
        "positionInitialMargin": "150.0",
        "openOrderInitialMargin": "50.0",
        "crossWalletBalance": "5000.0",
        "crossUnPnl": "50.0",
        "availableBalance": "4850.0",
        "maxWithdrawAmount": "4850.0",
        "marginAvailable": true,
        "updateTime": 1700000000001
      },
      {
        "asset": "BTC",
        "walletBalance": "0.5",
        "unrealizedProfit": "0.01",
        "marginBalance": "0.51",
        "maintMargin": "0.001",
        "initialMargin": "0.002",
        "positionInitialMargin": "0.0015",
        "openOrderInitialMargin": "0.0005",
        "crossWalletBalance": "0.5",
        "crossUnPnl": "0.01",
        "availableBalance": "0.498",
        "maxWithdrawAmount": "0.498",
        "marginAvailable": false,
        "updateTime": 1700000000002
      }
    ],
    "positions": []
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_INFO, data);
  auto result = AccountInfoParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<AccountInfoResponse>(result));
  const auto &info = std::get<AccountInfoResponse>(result);

  ASSERT_EQ(info.assets.size(), 2u);

  EXPECT_EQ(info.assets[0].asset, "USDT");
  EXPECT_EQ(info.assets[0].walletBalance.to_string(), "5000.0");
  EXPECT_EQ(info.assets[0].marginAvailable, true);
  EXPECT_EQ(info.assets[0].updateTime, 1700000000001u);

  EXPECT_EQ(info.assets[1].asset, "BTC");
  EXPECT_EQ(info.assets[1].walletBalance.to_string(), "0.5");
  EXPECT_EQ(info.assets[1].marginAvailable, false);
  EXPECT_EQ(info.assets[1].updateTime, 1700000000002u);
}

TEST(AccountRestApiParser, ParseAccountInfoWithPositions) {
  const std::string data = R"({
    "feeTier": 0,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "updateTime": 1700000000000,
    "assets": [],
    "positions": [
      {
        "symbol": "BTCUSDT",
        "initialMargin": "1000.0",
        "maintMargin": "500.0",
        "unrealizedProfit": "100.0",
        "positionInitialMargin": "900.0",
        "openOrderInitialMargin": "100.0",
        "leverage": 10,
        "isolated": false,
        "entryPrice": "50000.0",
        "maxNotional": "100000.0",
        "bidNotional": "0",
        "askNotional": "0",
        "positionSide": "BOTH",
        "positionAmt": "0.1",
        "notional": "5000.0",
        "isolatedWallet": "0",
        "updateTime": 1700000000001,
        "breakEvenPrice": "50100.0"
      },
      {
        "symbol": "ETHUSDT",
        "initialMargin": "300.0",
        "maintMargin": "150.0",
        "unrealizedProfit": "50.0",
        "positionInitialMargin": "270.0",
        "openOrderInitialMargin": "30.0",
        "leverage": 5,
        "isolated": true,
        "entryPrice": "3000.0",
        "maxNotional": "50000.0",
        "bidNotional": "0",
        "askNotional": "0",
        "positionSide": "LONG",
        "positionAmt": "1.0",
        "notional": "3000.0",
        "isolatedWallet": "600.0",
        "updateTime": 1700000000002,
        "breakEvenPrice": "3010.0"
      }
    ]
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_INFO, data);
  auto result = AccountInfoParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<AccountInfoResponse>(result));
  const auto &info = std::get<AccountInfoResponse>(result);

  ASSERT_EQ(info.positions.size(), 2u);

  EXPECT_EQ(info.positions[0].symbol, "BTCUSDT");
  EXPECT_EQ(info.positions[0].leverage, 10u);
  EXPECT_EQ(info.positions[0].isolated, false);
  EXPECT_EQ(info.positions[0].entryPrice.to_string(), "50000.0");
  EXPECT_EQ(info.positions[0].positionAmt.to_string(), "0.1");
  EXPECT_EQ(info.positions[0].positionSide, POSITION_SIDE::BOTH);

  EXPECT_EQ(info.positions[1].symbol, "ETHUSDT");
  EXPECT_EQ(info.positions[1].leverage, 5u);
  EXPECT_EQ(info.positions[1].isolated, true);
  EXPECT_EQ(info.positions[1].positionSide, POSITION_SIDE::LONG);
}

TEST(AccountRestApiParser, ParseAccountInfoErrorResponse) {
  const std::string data = R"({
    "code": -1000,
    "msg": "An unknown error occurred while processing the request."
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_INFO, data);
  auto result = AccountInfoParser::parse(msg);

  expectErrorParseMessage(result, "Error -1000");
  expectErrorParseMessage(result, "An unknown error occurred");
}

TEST(AccountRestApiParser, ParseAccountInfoInvalidJson) {
  const std::string data = "{invalid json}";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_INFO, data);
  auto result = AccountInfoParser::parse(msg);

  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
  expectErrorParseMessage(result, "JSON is empty");
}

TEST(AccountRestApiParser, ParseAccountInfoEmptyJson) {
  const std::string data = "{}";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_INFO, data);
  auto result = AccountInfoParser::parse(msg);

  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
  expectErrorParseMessage(result, "JSON is empty");
}

// ============================================================================
// AccountBalanceParser Tests
// ============================================================================

TEST(AccountRestApiParser, ParseAccountBalanceHappyPath) {
  const std::string data = R"([
    {
      "accountAlias": "main-account",
      "asset": "USDT",
      "balance": "10000.0",
      "crossWalletBalance": "9500.0",
      "crossUnPnl": "100.0",
      "availableBalance": "8500.0",
      "maxWithdrawAmount": "8500.0",
      "marginAvailable": true,
      "updateTime": 1700000000000
    },
    {
      "accountAlias": "main-account",
      "asset": "BTC",
      "balance": "1.5",
      "crossWalletBalance": "1.4",
      "crossUnPnl": "0.05",
      "availableBalance": "1.3",
      "maxWithdrawAmount": "1.3",
      "marginAvailable": false,
      "updateTime": 1700000000001
    }
  ])";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_BALANCE, data);
  auto result = AccountBalanceParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<AccountBalanceResponse>(result));
  const auto &balances = std::get<AccountBalanceResponse>(result);

  ASSERT_EQ(balances.balances.size(), 2u);

  EXPECT_EQ(balances.balances[0].accountAlias, "main-account");
  EXPECT_EQ(balances.balances[0].asset, "USDT");
  EXPECT_EQ(balances.balances[0].balance.to_string(), "10000.0");
  EXPECT_EQ(balances.balances[0].availableBalance.to_string(), "8500.0");
  EXPECT_EQ(balances.balances[0].marginAvailable, true);
  EXPECT_EQ(balances.balances[0].updateTime, 1700000000000u);

  EXPECT_EQ(balances.balances[1].asset, "BTC");
  EXPECT_EQ(balances.balances[1].balance.to_string(), "1.5");
  EXPECT_EQ(balances.balances[1].marginAvailable, false);
}

TEST(AccountRestApiParser, ParseAccountBalanceEmptyArray) {
  const std::string data = "[]";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_BALANCE, data);
  auto result = AccountBalanceParser::parse(msg);

  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
  expectErrorParseMessage(result, "JSON is empty");
}

TEST(AccountRestApiParser, ParseAccountBalanceNotArray) {
  const std::string data = R"({"error": "not an array"})";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_BALANCE, data);
  auto result = AccountBalanceParser::parse(msg);

  expectErrorParseMessage(result, "Expected array of balances");
}

TEST(AccountRestApiParser, ParseAccountBalanceErrorResponse) {
  const std::string data = R"({
    "code": -2015,
    "msg": "Invalid API-key, IP, or permissions for action."
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_BALANCE, data);
  auto result = AccountBalanceParser::parse(msg);

  expectErrorParseMessage(result, "Error -2015");
  expectErrorParseMessage(result, "Invalid API-key");
}

TEST(AccountRestApiParser, ParseAccountBalanceInvalidJson) {
  const std::string data = "[{invalid json}]";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_BALANCE, data);
  auto result = AccountBalanceParser::parse(msg);

  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
  expectErrorParseMessage(result, "JSON is empty");
}

TEST(AccountRestApiParser, ParseAccountBalancePartialData) {
  const std::string data = R"([
    {
      "asset": "USDT",
      "balance": "10000.0"
    }
  ])";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_BALANCE, data);
  auto result = AccountBalanceParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<AccountBalanceResponse>(result));
  const auto &balances = std::get<AccountBalanceResponse>(result);

  ASSERT_EQ(balances.balances.size(), 1u);
  EXPECT_EQ(balances.balances[0].asset, "USDT");
  EXPECT_EQ(balances.balances[0].balance.to_string(), "10000.0");
  // Other fields should have default values
  EXPECT_EQ(balances.balances[0].marginAvailable, false);
  EXPECT_EQ(balances.balances[0].updateTime, 0u);
}

// ============================================================================
// CommissionRateParser Tests
// ============================================================================

TEST(AccountRestApiParser, ParseCommissionRateHappyPath) {
  const std::string data = R"({
    "symbol": "BTCUSDT",
    "makerCommissionRate": "0.0002",
    "takerCommissionRate": "0.0004"
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::COMMISSION_RATE, data);
  auto result = CommissionRateParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<CommissionRateResponse>(result));
  const auto &rate = std::get<CommissionRateResponse>(result);

  EXPECT_EQ(rate.symbol, "BTCUSDT");
  EXPECT_EQ(rate.makerCommissionRate.to_string(), "0.0002");
  EXPECT_EQ(rate.takerCommissionRate.to_string(), "0.0004");
}

TEST(AccountRestApiParser, ParseCommissionRateErrorResponse) {
  const std::string data = R"({
    "code": -1121,
    "msg": "Invalid symbol."
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::COMMISSION_RATE, data);
  auto result = CommissionRateParser::parse(msg);

  expectErrorParseMessage(result, "Error -1121");
  expectErrorParseMessage(result, "Invalid symbol");
}

TEST(AccountRestApiParser, ParseCommissionRateInvalidJson) {
  const std::string data = "{invalid: json}";

  auto msg = makeMessage(ACCOUNT_API_TYPE::COMMISSION_RATE, data);
  auto result = CommissionRateParser::parse(msg);

  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
  expectErrorParseMessage(result, "JSON is empty");
}

TEST(AccountRestApiParser, ParseCommissionRateEmptyJson) {
  const std::string data = "{}";

  auto msg = makeMessage(ACCOUNT_API_TYPE::COMMISSION_RATE, data);
  auto result = CommissionRateParser::parse(msg);

  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
  expectErrorParseMessage(result, "JSON is empty");
}

TEST(AccountRestApiParser, ParseCommissionRateDifferentSymbols) {
  std::vector<std::string> symbols = {"BTCUSDT", "ETHUSDT", "BNBUSDT"};

  for (const auto &symbol : symbols) {
    std::string data = R"({
      "symbol": ")" + symbol +
                       R"(",
      "makerCommissionRate": "0.0001",
      "takerCommissionRate": "0.0002"
    })";

    auto msg = makeMessage(ACCOUNT_API_TYPE::COMMISSION_RATE, data);
    auto result = CommissionRateParser::parse(msg);

    ASSERT_TRUE(std::holds_alternative<CommissionRateResponse>(result));
    const auto &rate = std::get<CommissionRateResponse>(result);
    EXPECT_EQ(rate.symbol, symbol);
  }
}

TEST(AccountRestApiParser, ParseCommissionRateHighPrecision) {
  const std::string data = R"({
    "symbol": "BTCUSDT",
    "makerCommissionRate": "0.000123456789",
    "takerCommissionRate": "0.000987654321"
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::COMMISSION_RATE, data);
  auto result = CommissionRateParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<CommissionRateResponse>(result));
  const auto &rate = std::get<CommissionRateResponse>(result);

  // Max precision is 10
  EXPECT_EQ(rate.makerCommissionRate.to_string(), "0.0001234567");
  EXPECT_EQ(rate.takerCommissionRate.to_string(), "0.0009876543");
}

// ============================================================================
// Main Parser (AccountRestApiParser) Tests
// ============================================================================

TEST(AccountRestApiParser, ParseEmptyDataReturnsError) {
  JSONQuery emptyJson("");
  RestApiMessage msg(ACCOUNT_API_TYPE::ACCOUNT_INFO, emptyJson);

  auto result = AccountRestApiParser::parse(msg);

  expectErrorParseMessage(result, "RestApiMessage data is empty");
}

TEST(AccountRestApiParser, ParseUnknownTypeReturnsError) {
  JSONQuery json(R"({"test": "test2"})");
  RestApiMessage msg(ACCOUNT_API_TYPE::UNKNOWN, json);

  auto result = AccountRestApiParser::parse(msg);

  expectErrorParseMessage(result, "RestApiMessage type is invalid");
}

TEST(AccountRestApiParser, ParseDifferentApiTypes) {
  // Test that the main parser correctly dispatches to sub-parsers
  std::vector<ACCOUNT_API_TYPE> types = {ACCOUNT_API_TYPE::ACCOUNT_INFO,
                                         ACCOUNT_API_TYPE::COMMISSION_RATE};

  for (const auto &type : types) {
    JSONQuery json("{}");
    RestApiMessage msg(type, json);

    auto result = AccountRestApiParser::parse(msg);
    // Should not return "invalid type" error
    if (std::holds_alternative<ErrorParse>(result)) {
      auto error = std::get<ErrorParse>(result);
      EXPECT_EQ(error.parse_error.find("type is invalid"), std::string::npos);
    }
  }
}

// ============================================================================
// Edge Cases and Robustness Tests
// ============================================================================

TEST(AccountRestApiParser, ParseAccountInfoWithMixedValidInvalidFields) {
  const std::string data = R"({
    "feeTier": 2,
    "canTrade": true,
    "invalidField": "should be ignored",
    "updateTime": 1700000000000,
    "totalWalletBalance": "10000.0",
    "anotherInvalidField": 12345,
    "assets": []
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_INFO, data);
  auto result = AccountInfoParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<AccountInfoResponse>(result));
  const auto &info = std::get<AccountInfoResponse>(result);

  EXPECT_EQ(info.feeTier, 2u);
  EXPECT_EQ(info.canTrade, true);
  EXPECT_EQ(info.totalWalletBalance.to_string(), "10000.0");
}

TEST(AccountRestApiParser, ParseAccountBalanceWithZeroBalances) {
  const std::string data = R"([
    {
      "asset": "USDT",
      "balance": "0.0",
      "availableBalance": "0.0",
      "maxWithdrawAmount": "0.0",
      "updateTime": 1700000000000
    }
  ])";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_BALANCE, data);
  auto result = AccountBalanceParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<AccountBalanceResponse>(result));
  const auto &balances = std::get<AccountBalanceResponse>(result);

  ASSERT_EQ(balances.balances.size(), 1u);
  EXPECT_EQ(balances.balances[0].balance.to_string(), "0.0");
  EXPECT_EQ(balances.balances[0].availableBalance.to_string(), "0.0");
}

TEST(AccountRestApiParser, ParseCommissionRateZeroRates) {
  const std::string data = R"({
    "symbol": "BTCUSDT",
    "makerCommissionRate": "0.0",
    "takerCommissionRate": "0.0"
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::COMMISSION_RATE, data);
  auto result = CommissionRateParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<CommissionRateResponse>(result));
  const auto &rate = std::get<CommissionRateResponse>(result);

  EXPECT_EQ(rate.makerCommissionRate.to_string(), "0.0");
  EXPECT_EQ(rate.takerCommissionRate.to_string(), "0.0");
}

TEST(AccountRestApiParser, ParseAccountInfoVeryLargeNumbers) {
  const std::string data = R"({
    "feeTier": 0,
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": true,
    "totalWalletBalance": "999999999.999999999",
    "updateTime": 9999999999999,
    "assets": []
  })";

  auto msg = makeMessage(ACCOUNT_API_TYPE::ACCOUNT_INFO, data);
  auto result = AccountInfoParser::parse(msg);

  ASSERT_TRUE(std::holds_alternative<AccountInfoResponse>(result));
  const auto &info = std::get<AccountInfoResponse>(result);

  EXPECT_EQ(info.totalWalletBalance.to_string(), "999999999.999999999");
  EXPECT_EQ(info.updateTime, 9999999999999u);
}
