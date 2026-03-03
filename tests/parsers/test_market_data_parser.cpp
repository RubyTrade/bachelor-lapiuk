#include <gtest/gtest.h>

#include "core/controllers/market_data_utils.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/utils/fixed_num.hpp"

#include <string>

namespace {

Market::StreamMessage makeMessage(const std::string &stream,
                                  const std::string &jsonData) {
  JSONQuery json(jsonData);
  return Market::StreamMessage(stream, json);
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

TEST(MarketDataParser, ParseEmptyStreamReturnsError) {
  Market::TradeDataParser parser;
  auto result = parser.parse(makeMessage("", "{}"));
  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
}

TEST(MarketDataParser, ParseEmptyDataReturnsError) {
  Market::TradeDataParser parser;
  auto result = parser.parse(makeMessage("btcusdt@trade", ""));
  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
}

TEST(MarketDataParser, ParseTradeHappyPath) {
  Market::TradeDataParser parser;

  const std::string stream = "btcusdt@trade";
  const std::string data = R"({
        "e": "trade",
        "E": 1700000000000,
        "s": "BTCUSDT",
        "t": 12345,
        "p": "42000.10",
        "q": "0.005",
        "b": 999,
        "a": 111,
        "T": 1700000000001,
        "m": true,
        "M": true,
        "X": "MARKET"
    })";

  auto result = parser.parse(makeMessage(stream, data));

  ASSERT_TRUE(std::holds_alternative<Market::TradeData>(result));
  const auto &trade = std::get<Market::TradeData>(result);

  EXPECT_EQ(trade.eventType, MARKET_DATA_TYPE::TRADE);
  EXPECT_EQ(trade.symbol, "BTCUSDT");
  EXPECT_EQ(trade.tradeId, 12345u);
  EXPECT_EQ(trade.price.to_string(), "42000.10");
  EXPECT_EQ(trade.quantity.to_string(), "0.005");
  EXPECT_EQ(trade.isMarketMaker, true);
  EXPECT_EQ(trade.eventTime, 1700000000000u);
  EXPECT_EQ(trade.tradeTime, 1700000000001u);
}

TEST(MarketDataParser, ParseTradeMissingPriceReturnsError) {
  Market::TradeDataParser parser;

  const std::string data = R"({
        "e": "trade",
        "E": 1700000000000,
        "s": "BTCUSDT",
        "t": 12345,
        "q": "0.005",
        "T": 1700000000001,
        "m": true,
        "X": "MARKET"
    })";

  auto result = parser.parse(makeMessage("btcusdt@trade", data));
  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
}

TEST(MarketDataParser, ParseTradeWithDifferentOrderTypes) {
  Market::TradeDataParser parser;

  const std::string data = R"({
        "e": "trade",
        "E": 1700000000000,
        "s": "ETHUSDT",
        "t": 12346,
        "p": "3000.50",
        "q": "1.5",
        "T": 1700000000002,
        "m": false,
        "X": "LIMIT"
    })";

  auto result = parser.parse(makeMessage("ethusdt@trade", data));

  ASSERT_TRUE(std::holds_alternative<Market::TradeData>(result));
  const auto &trade = std::get<Market::TradeData>(result);

  EXPECT_EQ(trade.symbol, "ETHUSDT");
  EXPECT_EQ(trade.orderType, ORDER_TYPE::LIMIT);
  EXPECT_EQ(trade.isMarketMaker, false);
}

TEST(MarketDataParser, ParseMultipleTrades) {
  Market::TradeDataParser parser;

  std::vector<std::string> symbols = {"BTCUSDT", "ETHUSDT", "BNBUSDT"};

  for (const auto &symbol : symbols) {
    std::string stream = symbol + "@trade";
    std::string data = R"({
            "e": "trade",
            "E": 1700000000000,
            "s": ")" + symbol +
                       R"(",
            "t": 12345,
            "p": "100.0",
            "q": "1.0",
            "T": 1700000000001,
            "m": true,
            "X": "MARKET"
        })";

    auto result = parser.parse(makeMessage(stream, data));
    ASSERT_TRUE(std::holds_alternative<Market::TradeData>(result));
    const auto &trade = std::get<Market::TradeData>(result);
    EXPECT_EQ(trade.symbol, symbol);
  }
}

TEST(MarketDataParser, ParseTradeWithZeroQuantity) {
  Market::TradeDataParser parser;

  const std::string data = R"({
        "e": "trade",
        "E": 1700000000000,
        "s": "BTCUSDT",
        "t": 12345,
        "p": "42000.0",
        "q": "0.0",
        "T": 1700000000001,
        "m": false,
        "X": "MARKET"
    })";

  auto result = parser.parse(makeMessage("btcusdt@trade", data));

  ASSERT_TRUE(std::holds_alternative<Market::TradeData>(result));
  const auto &trade = std::get<Market::TradeData>(result);
  EXPECT_EQ(trade.quantity.to_string(), "0.0");
}

TEST(MarketDataParser, ParseTradeWithLargePrecision) {
  Market::TradeDataParser parser;

  const std::string data = R"({
        "e": "trade",
        "E": 1700000000000,
        "s": "BTCUSDT",
        "t": 12345,
        "p": "42000.123456789",
        "q": "0.000000001",
        "T": 1700000000001,
        "m": true,
        "X": "MARKET"
    })";

  auto result = parser.parse(makeMessage("btcusdt@trade", data));

  ASSERT_TRUE(std::holds_alternative<Market::TradeData>(result));
  const auto &trade = std::get<Market::TradeData>(result);
  EXPECT_EQ(trade.price.to_string(), "42000.123456789");
  EXPECT_EQ(trade.quantity.to_string(), "0.000000001");
}

TEST(MarketDataParser, ParseInvalidJsonReturnsError) {
  Market::TradeDataParser parser;

  auto result = parser.parse(makeMessage("btcusdt@trade", "{invalid json"));
  ASSERT_TRUE(std::holds_alternative<ErrorParse>(result));
}

TEST(MarketDataParser, ParseEmptyJsonReturnsError) {
  Market::TradeDataParser parser;

  auto result = parser.parse(makeMessage("btcusdt@trade", "{}"));
  EXPECT_TRUE(std::holds_alternative<ErrorParse>(result));
}

TEST(MarketDataParser, ParseAggTradeHappyPath) {
  Market::AggTradeDataParser parser;

  const std::string stream = "btcusdt@aggTrade";
  const std::string data = R"({
        "e": "aggTrade",
        "E": 1700000000000,
        "s": "BTCUSDT",
        "a": 777,
        "p": "42000.25",
        "q": "0.010",
        "nq": "0.010",
        "f": 10,
        "l": 11,
        "T": 1700000000002,
        "m": false,
        "M": true
    })";

  auto result = parser.parse(makeMessage(stream, data));

  ASSERT_TRUE(std::holds_alternative<Market::AggTradeData>(result));
  const auto &trade = std::get<Market::AggTradeData>(result);

  EXPECT_EQ(trade.eventType, MARKET_DATA_TYPE::AGG_TRADE);
  EXPECT_EQ(trade.symbol, "BTCUSDT");
  EXPECT_EQ(trade.aggTradeId, 777u);
  EXPECT_EQ(trade.price.to_string(), "42000.25");
  EXPECT_EQ(trade.quantity.to_string(), "0.010");
  EXPECT_EQ(trade.isMarketMaker, false);
}

TEST(MarketDataParser, ParseMarkPriceHappyPath) {
  Market::MarkPriceDataParser parser;

  const std::string stream = "btcusdt@markPrice";
  const std::string data = R"({
        "e": "markPriceUpdate",
        "E": 1700000000000,
        "s": "BTCUSDT",
        "p": "42001.5",
        "i": "42002.0",
        "P": "0.0010",
        "r": "0.00010000",
        "T": 1700000001000
    })";

  auto result = parser.parse(makeMessage(stream, data));

  ASSERT_TRUE(std::holds_alternative<Market::MarkPriceData>(result));
  const auto &mp = std::get<Market::MarkPriceData>(result);

  EXPECT_EQ(mp.symbol, "BTCUSDT");
  EXPECT_EQ(mp.markPrice.to_string(), "42001.5");
  EXPECT_EQ(mp.indexPrice.to_string(), "42002.0");
  EXPECT_EQ(mp.settlePrice.to_string(), "0.0010");
  EXPECT_EQ(mp.fundingRate.to_string(), "0.00010000");
}

TEST(MarketDataParser, ParseBookTickerHappyPath) {
  Market::BookTickerDataParser parser;

  const std::string stream = "btcusdt@bookTicker";
  const std::string data = R"({
    "e":"bookTicker",
    "u":9924360161244,
    "s":"BTCUSDT",
    "b":"68005.00",
    "B":"2.394",
    "a":"68005.10",
    "A":"0.857",
    "T":1771263510337,
    "E":1771263510337
  })";

  auto result = parser.parse(makeMessage(stream, data));

  ASSERT_TRUE(std::holds_alternative<Market::BookTickerData>(result));
  const auto &bt = std::get<Market::BookTickerData>(result);

  EXPECT_EQ(bt.symbol, "BTCUSDT");
  EXPECT_EQ(bt.bestBidPrice.to_string(), "68005.00");
  EXPECT_EQ(bt.bestBidQty.to_string(), "2.394");
  EXPECT_EQ(bt.bestAskPrice.to_string(), "68005.10");
  EXPECT_EQ(bt.bestAskQty.to_string(), "0.857");
}

TEST(MarketDataParser, ParseDepthHappyPath) {
  Market::DepthDataParser parser;

  const std::string stream = "btcusdt@depth5";
  const std::string data = R"({
        "e": "depthUpdate",
        "E": 1700000000000,
        "T": 1700000000001,
        "s": "BTCUSDT",
        "U": 1,
        "u": 2,
        "pu": 0,
        "b": [
            ["41999.0", "0.1"],
            ["41998.5", "0.2"]
        ],
        "a": [
            ["42000.0", "0.3"],
            ["42000.5", "0.4"]
        ]
    })";

  auto result = parser.parse(makeMessage(stream, data));

  ASSERT_TRUE(std::holds_alternative<Market::DepthData>(result));
  const auto &depth = std::get<Market::DepthData>(result);

  ASSERT_EQ(depth.bids.size(), 2u);
  ASSERT_EQ(depth.asks.size(), 2u);

  EXPECT_EQ(depth.bids.at(0).first.to_string(), "41999.0");
  EXPECT_EQ(depth.bids.at(0).second.to_string(), "0.1");

  EXPECT_EQ(depth.asks.at(1).first.to_string(), "42000.5");
  EXPECT_EQ(depth.asks.at(1).second.to_string(), "0.4");
}

TEST(MarketDataParser, ParseDepthMissingBidsReturnsError) {
  Market::DepthDataParser parser;

  const std::string stream = "btcusdt@depth5";
  const std::string data = R"({
        "e": "depthUpdate",
        "E": 1700000000000,
        "T": 1700000000001,
        "s": "BTCUSDT",
        "U": 1,
        "u": 2,
        "pu": 0,
        "a": [["42000.0", "0.3"]]
    })";

  auto result = parser.parse(makeMessage(stream, data));
  expectErrorParseMessage(result, "bids are not parsed");
}

TEST(MarketDataParser, ParseDepthWrongArrayShapeReturnsError) {
  Market::DepthDataParser parser;

  const std::string stream = "btcusdt@depth5";
  const std::string data = R"({
        "e": "depthUpdate",
        "E": 1700000000000,
        "T": 1700000000001,
        "s": "BTCUSDT",
        "U": 1,
        "u": 2,
        "pu": 0,
        "b": [["41999.0"]],
        "a": [["42000.0", "0.3"]]
    })";

  auto result = parser.parse(makeMessage(stream, data));
  expectErrorParseMessage(result, "array of bids is not parsed");
}

TEST(MarketDataParser, ParseDepthWrongTypesReturnsError) {
  Market::DepthDataParser parser;

  const std::string stream = "btcusdt@depth5";
  const std::string data = R"({
        "e": "depthUpdate",
        "E": 1700000000000,
        "T": 1700000000001,
        "s": "BTCUSDT",
        "U": 1,
        "u": 2,
        "pu": 0,
        "b": [[41999.0, "0.1"]],
        "a": [["42000.0", "0.3"]]
    })";

  auto result = parser.parse(makeMessage(stream, data));
  expectErrorParseMessage(result, "array of bids is not parsed");
}
