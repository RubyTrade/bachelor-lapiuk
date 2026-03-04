#include <gtest/gtest.h>

#include "core/controllers/trading_stream_utils.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/parsers/trading_stream_parser.hpp"

#include <string>

namespace {

using namespace Trading;

ResultMessage makeResultMessage(const std::string &id, uint16_t statusCode,
                                const std::string &resultJson) {
  ResultMessage msg(id, statusCode);
  msg.type = MESSAGE_TYPE::SUCCESS;
  msg.result_msg = JSONQuery(resultJson);
  return msg;
}

ResultMessage makeErrorMessage(const std::string &id, uint16_t statusCode,
                               int errorCode, const std::string &errorMsg) {
  ResultMessage msg(id, statusCode);
  msg.type = MESSAGE_TYPE::ERROR;
  msg.error_code = errorCode;
  msg.error_msg = errorMsg;
  return msg;
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
// AccountStatusParser Tests
// ============================================================================

TEST(TradingStreamParser, ParseAccountStatusHappyPath) {
  const std::string data = R"({
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": false,
    "updateTime": 1700000000000
  })";

  auto msg = makeResultMessage("account.status.1", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_STATUS, msg});

  ASSERT_TRUE(std::holds_alternative<AccountStatusResponse>(result));
  const auto &status = std::get<AccountStatusResponse>(result);

  EXPECT_EQ(status.apiType, TRADE_STREAM_METHOD::ACCOUNT_STATUS);
  EXPECT_TRUE(status.canTrade);
  EXPECT_TRUE(status.canDeposit);
  EXPECT_FALSE(status.canWithdraw);
  EXPECT_EQ(status.updateTime, 1700000000000u);
}

TEST(TradingStreamParser, ParseAccountStatusMissingFields) {
  const std::string data = R"({
    "canTrade": true,
    "canDeposit": false
  })";

  auto msg = makeResultMessage("account.status.2", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_STATUS, msg});

  ASSERT_TRUE(std::holds_alternative<AccountStatusResponse>(result));
  const auto &status = std::get<AccountStatusResponse>(result);

  EXPECT_TRUE(status.canTrade);
  EXPECT_FALSE(status.canDeposit);
  EXPECT_FALSE(status.canWithdraw);
  EXPECT_EQ(status.updateTime, 0u);
}

// ============================================================================
// AccountBalanceParser Tests
// ============================================================================

TEST(TradingStreamParser, ParseAccountBalanceHappyPath) {
  const std::string data = R"([
    {
      "asset": "USDT",
      "balance": "10000.50",
      "crossWalletBalance": "9500.0",
      "crossUnPnl": "100.5",
      "availableBalance": "8500.0",
      "maxWithdrawAmount": "8000.0",
      "marginAvailable": true,
      "updateTime": 1700000000000
    },
    {
      "asset": "BTC",
      "balance": "0.5",
      "crossWalletBalance": "0.45",
      "crossUnPnl": "0.05",
      "availableBalance": "0.4",
      "maxWithdrawAmount": "0.35",
      "marginAvailable": false,
      "updateTime": 1700000000001
    }
  ])";

  auto msg = makeResultMessage("account.balance.1", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_BALANCE, msg});

  ASSERT_TRUE(std::holds_alternative<AccountBalanceResponse>(result));
  const auto &balance = std::get<AccountBalanceResponse>(result);

  EXPECT_EQ(balance.apiType, TRADE_STREAM_METHOD::ACCOUNT_BALANCE);
  ASSERT_EQ(balance.balances.size(), 2u);

  EXPECT_EQ(balance.balances[0].asset, "USDT");
  EXPECT_EQ(balance.balances[0].balance.to_string(), "10000.50");
  EXPECT_EQ(balance.balances[0].crossWalletBalance.to_string(), "9500.0");
  EXPECT_TRUE(balance.balances[0].marginAvailable);

  EXPECT_EQ(balance.balances[1].asset, "BTC");
  EXPECT_EQ(balance.balances[1].balance.to_string(), "0.5");
  EXPECT_FALSE(balance.balances[1].marginAvailable);
}

TEST(TradingStreamParser, ParseAccountBalanceEmptyArray) {
  const std::string data = R"([])";

  auto msg = makeResultMessage("account.balance.2", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_BALANCE, msg});

  ASSERT_TRUE(std::holds_alternative<ErrorParse>(result));
}

TEST(TradingStreamParser, ParseAccountBalanceNotArray) {
  const std::string data = R"({
    "asset": "USDT",
    "balance": "10000.0"
  })";

  auto msg = makeResultMessage("account.balance.3", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_BALANCE, msg});

  expectErrorParseMessage(result, "expected array");
}

// ============================================================================
// OrderPlaceParser Tests
// ============================================================================

TEST(TradingStreamParser, ParseOrderPlaceHappyPath) {
  const std::string data = R"({
    "symbol": "BTCUSDT",
    "orderId": 123456789,
    "clientOrderId": "BTCUSDT_LONG_1234567890_BUY_1",
    "price": "50000.0",
    "origQty": "0.1",
    "executedQty": "0.0",
    "cumQuote": "0.0",
    "type": "LIMIT",
    "side": "BUY",
    "positionSide": "LONG",
    "timeInForce": "GTC",
    "status": "NEW",
    "updateTime": 1700000000000,
    "workingTime": 1700000000001
  })";

  auto msg = makeResultMessage("order.place.1", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_PLACE, msg});

  ASSERT_TRUE(std::holds_alternative<OrderPlaceResponse>(result));
  const auto &order = std::get<OrderPlaceResponse>(result);

  EXPECT_EQ(order.apiType, TRADE_STREAM_METHOD::ORDER_PLACE);
  EXPECT_EQ(order.symbol, "BTCUSDT");
  EXPECT_EQ(order.orderId, 123456789);
  EXPECT_EQ(order.clientOrderId, "BTCUSDT_LONG_1234567890_BUY_1");
  EXPECT_EQ(order.price.to_string(), "50000.0");
  EXPECT_EQ(order.origQty.to_string(), "0.1");
  EXPECT_EQ(order.executedQty.to_string(), "0.0");
  EXPECT_EQ(order.type, ORDER_TYPE::LIMIT);
  EXPECT_EQ(order.side, ORDER_SIDE::BUY);
  EXPECT_EQ(order.positionSide, POSITION_SIDE::LONG);
  EXPECT_EQ(order.timeInForce, TIME_IN_FORCE::GTC);
  EXPECT_EQ(order.status, "NEW");
  EXPECT_EQ(order.updateTime, 1700000000000u);
  EXPECT_EQ(order.workingTime, 1700000000001u);
}

TEST(TradingStreamParser, ParseOrderPlaceMarketOrder) {
  const std::string data = R"({
    "symbol": "ETHUSDT",
    "orderId": 987654321,
    "clientOrderId": "ETHUSDT_BOTH_9876543210_SELL_2",
    "price": "0.0",
    "origQty": "1.0",
    "executedQty": "1.0",
    "cumQuote": "3000.0",
    "type": "MARKET",
    "side": "SELL",
    "positionSide": "BOTH",
    "timeInForce": "GTC",
    "status": "FILLED",
    "updateTime": 1700000000000,
    "workingTime": 1700000000000
  })";

  auto msg = makeResultMessage("order.place.2", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_PLACE, msg});

  ASSERT_TRUE(std::holds_alternative<OrderPlaceResponse>(result));
  const auto &order = std::get<OrderPlaceResponse>(result);

  EXPECT_EQ(order.symbol, "ETHUSDT");
  EXPECT_EQ(order.type, ORDER_TYPE::MARKET);
  EXPECT_EQ(order.side, ORDER_SIDE::SELL);
  EXPECT_EQ(order.positionSide, POSITION_SIDE::BOTH);
  EXPECT_EQ(order.status, "FILLED");
  EXPECT_EQ(order.executedQty.to_string(), "1.0");
  EXPECT_EQ(order.cumQuote.to_string(), "3000.0");
}

// ============================================================================
// OrderModifyParser Tests
// ============================================================================

TEST(TradingStreamParser, ParseOrderModifyHappyPath) {
  const std::string data = R"({
    "symbol": "BTCUSDT",
    "orderId": 123456789,
    "clientOrderId": "BTCUSDT_LONG_1234567890_BUY_1",
    "price": "51000.0",
    "origQty": "0.2",
    "status": "NEW",
    "updateTime": 1700000000000
  })";

  auto msg = makeResultMessage("order.modify.1", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_MODIFY, msg});

  ASSERT_TRUE(std::holds_alternative<OrderModifyResponse>(result));
  const auto &order = std::get<OrderModifyResponse>(result);

  EXPECT_EQ(order.apiType, TRADE_STREAM_METHOD::ORDER_MODIFY);
  EXPECT_EQ(order.symbol, "BTCUSDT");
  EXPECT_EQ(order.orderId, 123456789);
  EXPECT_EQ(order.clientOrderId, "BTCUSDT_LONG_1234567890_BUY_1");
  EXPECT_EQ(order.price.to_string(), "51000.0");
  EXPECT_EQ(order.origQty.to_string(), "0.2");
  EXPECT_EQ(order.status, "NEW");
  EXPECT_EQ(order.updateTime, 1700000000000u);
}

// ============================================================================
// OrderCancelParser Tests
// ============================================================================

TEST(TradingStreamParser, ParseOrderCancelHappyPath) {
  const std::string data = R"({
    "symbol": "BTCUSDT",
    "orderId": 123456789,
    "clientOrderId": "BTCUSDT_LONG_1234567890_BUY_1",
    "status": "CANCELED",
    "updateTime": 1700000000000
  })";

  auto msg = makeResultMessage("order.cancel.1", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_CANCEL, msg});

  ASSERT_TRUE(std::holds_alternative<OrderCancelResponse>(result));
  const auto &order = std::get<OrderCancelResponse>(result);

  EXPECT_EQ(order.apiType, TRADE_STREAM_METHOD::ORDER_CANCEL);
  EXPECT_EQ(order.symbol, "BTCUSDT");
  EXPECT_EQ(order.orderId, 123456789);
  EXPECT_EQ(order.clientOrderId, "BTCUSDT_LONG_1234567890_BUY_1");
  EXPECT_EQ(order.status, "CANCELED");
  EXPECT_EQ(order.updateTime, 1700000000000u);
}

// ============================================================================
// OrderStatusParser Tests
// ============================================================================

TEST(TradingStreamParser, ParseOrderStatusHappyPath) {
  const std::string data = R"({
    "symbol": "BTCUSDT",
    "orderId": 123456789,
    "clientOrderId": "BTCUSDT_LONG_1234567890_BUY_1",
    "price": "50000.0",
    "origQty": "0.1",
    "executedQty": "0.05",
    "cumQuote": "2500.0",
    "type": "LIMIT",
    "side": "BUY",
    "positionSide": "LONG",
    "timeInForce": "GTC",
    "status": "PARTIALLY_FILLED",
    "avgPrice": "50000.0",
    "updateTime": 1700000000000,
    "workingTime": 1699999999999
  })";

  auto msg = makeResultMessage("order.status.1", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_STATUS, msg});

  ASSERT_TRUE(std::holds_alternative<OrderStatusResponse>(result));
  const auto &order = std::get<OrderStatusResponse>(result);

  EXPECT_EQ(order.apiType, TRADE_STREAM_METHOD::ORDER_STATUS);
  EXPECT_EQ(order.symbol, "BTCUSDT");
  EXPECT_EQ(order.orderId, 123456789);
  EXPECT_EQ(order.clientOrderId, "BTCUSDT_LONG_1234567890_BUY_1");
  EXPECT_EQ(order.price.to_string(), "50000.0");
  EXPECT_EQ(order.origQty.to_string(), "0.1");
  EXPECT_EQ(order.executedQty.to_string(), "0.05");
  EXPECT_EQ(order.cumQuote.to_string(), "2500.0");
  EXPECT_EQ(order.avgPrice.to_string(), "50000.0");
  EXPECT_EQ(order.type, ORDER_TYPE::LIMIT);
  EXPECT_EQ(order.side, ORDER_SIDE::BUY);
  EXPECT_EQ(order.positionSide, POSITION_SIDE::LONG);
  EXPECT_EQ(order.timeInForce, TIME_IN_FORCE::GTC);
  EXPECT_EQ(order.status, "PARTIALLY_FILLED");
  EXPECT_EQ(order.updateTime, 1700000000000u);
  EXPECT_EQ(order.workingTime, 1699999999999u);
}

TEST(TradingStreamParser, ParseOrderStatusFilledOrder) {
  const std::string data = R"({
    "symbol": "ETHUSDT",
    "orderId": 987654321,
    "clientOrderId": "ETHUSDT_SHORT_9876543210_SELL_2",
    "price": "3000.0",
    "origQty": "2.0",
    "executedQty": "2.0",
    "cumQuote": "6000.0",
    "type": "LIMIT",
    "side": "SELL",
    "positionSide": "SHORT",
    "timeInForce": "GTC",
    "status": "FILLED",
    "avgPrice": "3000.0",
    "updateTime": 1700000000000,
    "workingTime": 1699999999999
  })";

  auto msg = makeResultMessage("order.status.2", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_STATUS, msg});

  ASSERT_TRUE(std::holds_alternative<OrderStatusResponse>(result));
  const auto &order = std::get<OrderStatusResponse>(result);

  EXPECT_EQ(order.symbol, "ETHUSDT");
  EXPECT_EQ(order.positionSide, POSITION_SIDE::SHORT);
  EXPECT_EQ(order.status, "FILLED");
  EXPECT_EQ(order.executedQty.to_string(), "2.0");
  EXPECT_EQ(order.avgPrice.to_string(), "3000.0");
}

// ============================================================================
// AccountPositionParser Tests
// ============================================================================

TEST(TradingStreamParser, ParseAccountPositionHappyPath) {
  const std::string data = R"([
    {
      "symbol": "BTCUSDT",
      "positionAmt": "0.1",
      "entryPrice": "50000.0",
      "breakEvenPrice": "50100.0",
      "markPrice": "51000.0",
      "unRealizedProfit": "100.0",
      "liquidationPrice": "45000.0",
      "leverage": 10,
      "maxNotionalValue": "100000.0",
      "marginType": "cross",
      "isolated": false,
      "positionSide": "LONG",
      "notional": "5100.0",
      "isolatedWallet": "0",
      "updateTime": 1700000000000
    },
    {
      "symbol": "ETHUSDT",
      "positionAmt": "1.0",
      "entryPrice": "3000.0",
      "breakEvenPrice": "3010.0",
      "markPrice": "3050.0",
      "unRealizedProfit": "50.0",
      "liquidationPrice": "2500.0",
      "leverage": 5,
      "maxNotionalValue": "50000.0",
      "marginType": "isolated",
      "isolated": true,
      "positionSide": "BOTH",
      "notional": "3050.0",
      "isolatedWallet": "1000.0",
      "updateTime": 1700000000001
    }
  ])";

  auto msg = makeResultMessage("account.position.1", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_POSITION, msg});

  ASSERT_TRUE(std::holds_alternative<AccountPositionResponse>(result));
  const auto &positions = std::get<AccountPositionResponse>(result);

  EXPECT_EQ(positions.apiType, TRADE_STREAM_METHOD::ACCOUNT_POSITION);
  ASSERT_EQ(positions.positions.size(), 2u);

  // First position
  EXPECT_EQ(positions.positions[0].symbol, "BTCUSDT");
  EXPECT_EQ(positions.positions[0].positionAmt.to_string(), "0.1");
  EXPECT_EQ(positions.positions[0].entryPrice.to_string(), "50000.0");
  EXPECT_EQ(positions.positions[0].breakEvenPrice.to_string(), "50100.0");
  EXPECT_EQ(positions.positions[0].markPrice.to_string(), "51000.0");
  EXPECT_EQ(positions.positions[0].unRealizedProfit.to_string(), "100.0");
  EXPECT_EQ(positions.positions[0].liquidationPrice.to_string(), "45000.0");
  EXPECT_EQ(positions.positions[0].leverage, 10u);
  EXPECT_EQ(positions.positions[0].marginType, MARGIN_TYPE::CROSSED);
  EXPECT_FALSE(positions.positions[0].isolated);
  EXPECT_EQ(positions.positions[0].positionSide, POSITION_SIDE::LONG);

  // Second position
  EXPECT_EQ(positions.positions[1].symbol, "ETHUSDT");
  EXPECT_EQ(positions.positions[1].positionAmt.to_string(), "1.0");
  EXPECT_EQ(positions.positions[1].entryPrice.to_string(), "3000.0");
  EXPECT_EQ(positions.positions[1].leverage, 5u);
  EXPECT_EQ(positions.positions[1].marginType, MARGIN_TYPE::ISOLATED);
  EXPECT_TRUE(positions.positions[1].isolated);
  EXPECT_EQ(positions.positions[1].positionSide, POSITION_SIDE::BOTH);
  EXPECT_EQ(positions.positions[1].isolatedWallet.to_string(), "1000.0");
}

TEST(TradingStreamParser, ParseAccountPositionEmptyArray) {
  const std::string data = R"([])";

  auto msg = makeResultMessage("account.position.2", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_POSITION, msg});

  ASSERT_TRUE(std::holds_alternative<ErrorParse>(result));
}

TEST(TradingStreamParser, ParseAccountPositionNotArray) {
  const std::string data = R"({
    "symbol": "BTCUSDT",
    "positionAmt": "0.1"
  })";

  auto msg = makeResultMessage("account.position.3", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_POSITION, msg});

  expectErrorParseMessage(result, "expected array");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(TradingStreamParser, ParseErrorMessage) {
  auto msg =
      makeErrorMessage("order.place.error", 400, -1001, "Invalid symbol");
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_PLACE, msg});

  expectErrorParseMessage(result, "Invalid symbol");
}

TEST(TradingStreamParser, ParseInvalidJSON) {
  const std::string data = R"({invalid json})";

  auto msg = makeResultMessage("test.1", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_PLACE, msg});

  ASSERT_TRUE(std::holds_alternative<ErrorParse>(result));
}

// ============================================================================
// Method Detection Tests
// ============================================================================

TEST(TradingStreamParser, DetectMethodFromOrderId) {
  const std::string data = R"({
    "orderId": 123456789,
    "symbol": "BTCUSDT",
    "clientOrderId": "test",
    "price": "50000.0",
    "origQty": "0.1",
    "executedQty": "0.0",
    "cumQuote": "0.0",
    "type": "LIMIT",
    "side": "BUY",
    "positionSide": "LONG",
    "timeInForce": "GTC",
    "status": "NEW",
    "updateTime": 1700000000000,
    "workingTime": 1700000000000
  })";

  auto msg = makeResultMessage("order.place.xyz", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ORDER_PLACE, msg});

  ASSERT_TRUE(std::holds_alternative<OrderPlaceResponse>(result));
}

TEST(TradingStreamParser, DetectMethodFromArrayStructure) {
  const std::string data = R"([
    {
      "asset": "USDT",
      "balance": "10000.0",
      "crossWalletBalance": "9500.0"
    }
  ])";

  auto msg = makeResultMessage("random.id.123", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_BALANCE, msg});

  ASSERT_TRUE(std::holds_alternative<AccountBalanceResponse>(result));
}

TEST(TradingStreamParser, DetectMethodFromCanTradeField) {
  const std::string data = R"({
    "canTrade": true,
    "canDeposit": true,
    "canWithdraw": false
  })";

  auto msg = makeResultMessage("random.status.456", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::ACCOUNT_STATUS, msg});

  ASSERT_TRUE(std::holds_alternative<AccountStatusResponse>(result));
}

TEST(TradingStreamParser, UnknownMethodReturnsError) {
  const std::string data = R"({
    "unknownField": "value"
  })";

  auto msg = makeResultMessage("unknown.method", 200, data);
  auto result =
      TradingStreamParser::parse({TRADE_STREAM_METHOD::INVALID_METHOD, msg});

  expectErrorParseMessage(result, "Unknown TRADE_STREAM_METHOD");
}
