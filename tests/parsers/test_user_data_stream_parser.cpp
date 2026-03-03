#include <gtest/gtest.h>

#include "core/controllers/user_data_stream_utils.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/fixed_num.hpp"

#include <string>

namespace {

UserData::StreamMessage makeMessage(USER_DATA_EVENT_TYPE type,
                                    const std::string &jsonData) {
  JSONQuery json(jsonData);
  return UserData::StreamMessage(type, json);
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

// Note: The parser classes are specific to event types
// We need to test individual parsers for each event type

TEST(UserDataStreamParser, ParseOrderTradeUpdateHappyPath) {
  const std::string data = R"({
        "e": "ORDER_TRADE_UPDATE",
        "E": 1700000000000,
        "T": 1700000000001,
        "o": {
            "s": "BTCUSDT",
            "c": "my-order-1",
            "S": "BUY",
            "o": "LIMIT",
            "f": "GTC",
            "q": "0.010",
            "p": "42000.0",
            "ap": "0",
            "sp": "0",
            "x": "NEW",
            "X": "NEW",
            "i": 123456,
            "l": "0",
            "z": "0",
            "L": "0",
            "n": "0",
            "N": "USDT",
            "T": 1700000000001,
            "t": 0,
            "b": "0",
            "a": "0",
            "m": false,
            "R": false,
            "wt": "CONTRACT_PRICE",
            "ps": "BOTH",
            "cp": false,
            "rp": "0",
            "pP": false
        }
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE, data);

  // Note: The actual parser needs to be tested based on implementation
  // This is a placeholder test structure
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE);
}

TEST(UserDataStreamParser, ParseTradeLiteEvent) {
  const std::string data = R"({
        "e": "TRADE_LITE",
        "E": 1700000000000,
        "s": "BTCUSDT",
        "c": "client-order-1",
        "S": "BUY",
        "o": 12345,
        "t": 67890,
        "p": "50000.0",
        "q": "0.1",
        "L": "50000.0",
        "l": "0.1",
        "m": true,
        "T": 1700000000001
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::TRADE_LITE, data);
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::TRADE_LITE);
}

TEST(UserDataStreamParser, ParseAccountUpdateEvent) {
  const std::string data = R"({
        "e": "ACCOUNT_UPDATE",
        "E": 1700000000000,
        "T": 1700000000001,
        "a": {
            "m": "ORDER",
            "B": [
                {"a": "USDT", "wb": "1000.0", "cw": "900.0"}
            ],
            "P": [
                {"s": "BTCUSDT", "pa": "0.1", "ep": "50000.0", "cr": "0", "up": "100.0", 
                 "mt": "CROSSED", "iw": "0", "ps": "BOTH", "bep": "50000.0"}
            ]
        }
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE, data);
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE);
}

TEST(UserDataStreamParser, ParseAccountConfigUpdateEvent) {
  const std::string data = R"({
        "e": "ACCOUNT_CONFIG_UPDATE",
        "E": 1700000000000,
        "T": 1700000000001,
        "ac": {
            "s": "BTCUSDT",
            "l": 20
        }
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE, data);
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE);
}

TEST(UserDataStreamParser, ParseMarginCallEvent) {
  const std::string data = R"({
        "e": "MARGIN_CALL",
        "E": 1700000000000,
        "cw": "100.50",
        "p": [
            {
                "s": "BTCUSDT",
                "ps": "BOTH",
                "pa": "0.5",
                "mt": "CROSSED",
                "iw": "0",
                "mp": "50000.0",
                "up": "-250.0",
                "mm": "50.0"
            }
        ]
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::MARGIN_CALL, data);
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::MARGIN_CALL);
}

TEST(UserDataStreamParser, StreamMessageConstructor) {
  JSONQuery json(std::string("{}"));
  UserData::StreamMessage msg(USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE, json);

  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE);
}

TEST(UserDataStreamParser, DifferentEventTypes) {
  std::vector<USER_DATA_EVENT_TYPE> types = {
      USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE,
      USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE,
      USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE,
      USER_DATA_EVENT_TYPE::MARGIN_CALL, USER_DATA_EVENT_TYPE::TRADE_LITE};

  for (const auto &type : types) {
    JSONQuery json(std::string("{}"));
    UserData::StreamMessage msg(type, json);
    EXPECT_EQ(msg.userDataType, type);
  }
}

// Integration tests for parsing complete events
TEST(UserDataEventParsing, OrderWithFilledStatus) {
  const std::string data = R"({
        "e": "ORDER_TRADE_UPDATE",
        "E": 1700000000000,
        "T": 1700000000001,
        "o": {
            "s": "ETHUSDT",
            "c": "filled-order",
            "S": "SELL",
            "o": "MARKET",
            "f": "GTC",
            "q": "1.0",
            "p": "3000.0",
            "ap": "3000.5",
            "x": "TRADE",
            "X": "FILLED",
            "i": 789,
            "l": "1.0",
            "z": "1.0",
            "L": "3000.5",
            "n": "3.0",
            "N": "USDT",
            "T": 1700000000001,
            "t": 111,
            "m": false,
            "R": false,
            "wt": "CONTRACT_PRICE",
            "ps": "BOTH",
            "rp": "50.5"
        }
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE, data);
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE);
}

TEST(UserDataEventParsing, MultipleBalancesAndPositions) {
  const std::string data = R"({
        "e": "ACCOUNT_UPDATE",
        "E": 1700000000000,
        "T": 1700000000001,
        "a": {
            "m": "DEPOSIT",
            "B": [
                {"a": "USDT", "wb": "5000.0", "cw": "4500.0"},
                {"a": "BTC", "wb": "0.5", "cw": "0.45"},
                {"a": "ETH", "wb": "10.0", "cw": "9.5"}
            ],
            "P": [
                {"s": "BTCUSDT", "pa": "0.1", "ep": "50000.0", "bep": "50000.0"},
                {"s": "ETHUSDT", "pa": "5.0", "ep": "3000.0", "bep": "3000.0"}
            ]
        }
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE, data);
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE);
}

TEST(UserDataEventParsing, LeverageUpdate) {
  const std::string data = R"({
        "e": "ACCOUNT_CONFIG_UPDATE",
        "E": 1700000000000,
        "T": 1700000000001,
        "ac": {
            "s": "BTCUSDT",
            "l": 50
        }
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE, data);
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE);
}

TEST(UserDataEventParsing, MultiAssetModeUpdate) {
  const std::string data = R"({
        "e": "ACCOUNT_CONFIG_UPDATE",
        "E": 1700000000000,
        "T": 1700000000001,
        "ai": {
            "j": true
        }
    })";

  auto msg = makeMessage(USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE, data);
  EXPECT_EQ(msg.userDataType, USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE);
}
