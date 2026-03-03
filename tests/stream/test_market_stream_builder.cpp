#include <gtest/gtest.h>

#include "core/stream/market_stream.hpp"
#include "core/utils/json.hpp"

#include <string>

// Tests for MarketStreamQueryBuilder

// Basic construction and method tests
TEST(MarketStreamBuilder, ConstructWithMethod) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("BTCUSDT");
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());
}

TEST(MarketStreamBuilder, SetMethodChanges) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.setMethod(MARKET_STREAM_METHOD::UNSUBSCRIBE);
  builder.add_trade_symbol("BTCUSDT");
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());
}

TEST(MarketStreamBuilder, ListSubscriptionsNoParams) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::LIST_SUBSCRIPTIONS);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  // LIST_SUBSCRIPTIONS should not require params
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("\"method\""), std::string::npos);
    EXPECT_NE(json_str.find("\"id\""), std::string::npos);
  }
}

// Subscribe to single symbols
TEST(MarketStreamBuilder, AddSingleTradeSymbol) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("btcusdt");
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("btcusdt@trade"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddSingleAggTradeSymbol) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_aggTrade_symbol("ethusdt");
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("ethusdt@aggTrade"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddMarkPriceSymbol) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_markPrice_symbol("btcusdt", true);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    // Should contain markPrice with fast update
    EXPECT_NE(json_str.find("btcusdt@markPrice"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddMarkPriceSlowUpdate) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_markPrice_symbol("ethusdt", false);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("ethusdt@markPrice"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddDiffDepthSymbol) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_diffDepth_symbol("btcusdt", true);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("btcusdt@depth"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddPartDepthSmallLevel) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_partDepth_symbol(
      "btcusdt", MarketStreamQueryBuilder::DEPTH_LEVELS::SMALL, true);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("btcusdt@depth"), std::string::npos);
    EXPECT_NE(json_str.find("5"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddPartDepthMediumLevel) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_partDepth_symbol(
      "ethusdt", MarketStreamQueryBuilder::DEPTH_LEVELS::MEDIUM, false);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("ethusdt@depth"), std::string::npos);
    EXPECT_NE(json_str.find("10"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddPartDepthLargeLevel) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_partDepth_symbol(
      "solusdt", MarketStreamQueryBuilder::DEPTH_LEVELS::LARGE, true);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("solusdt@depth"), std::string::npos);
    EXPECT_NE(json_str.find("20"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddBookTickerSymbol) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_bookTicker_symbol("btcusdt");
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("btcusdt@bookTicker"), std::string::npos);
  }
}

// Subscribe to multiple symbols
TEST(MarketStreamBuilder, AddMultipleSymbols) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("btcusdt");
  builder.add_trade_symbol("ethusdt");
  builder.add_aggTrade_symbol("solusdt");
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("btcusdt@trade"), std::string::npos);
    EXPECT_NE(json_str.find("ethusdt@trade"), std::string::npos);
    EXPECT_NE(json_str.find("solusdt@aggTrade"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, AddMixedDataTypes) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("btcusdt");
  builder.add_markPrice_symbol("btcusdt", true);
  builder.add_bookTicker_symbol("btcusdt");
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("@trade"), std::string::npos);
    EXPECT_NE(json_str.find("@markPrice"), std::string::npos);
    EXPECT_NE(json_str.find("@bookTicker"), std::string::npos);
  }
}

// Unsubscribe operations
TEST(MarketStreamBuilder, UnsubscribeFromSymbols) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::UNSUBSCRIBE);
  builder.add_trade_symbol("btcusdt");
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("UNSUBSCRIBE"), std::string::npos);
    EXPECT_NE(json_str.find("btcusdt@trade"), std::string::npos);
  }
}

// Set property tests
TEST(MarketStreamBuilder, SetCombinedPropertyTrue) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SET_PROPERTY);
  builder.set_combined_property(true);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("SET_PROPERTY"), std::string::npos);
    EXPECT_NE(json_str.find("combined"), std::string::npos);
    EXPECT_NE(json_str.find("true"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, SetCombinedPropertyFalse) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SET_PROPERTY);
  builder.set_combined_property(false);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("false"), std::string::npos);
  }
}

// Get property tests
TEST(MarketStreamBuilder, GetProperty) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::GET_PROPERTY);
  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("GET_PROPERTY"), std::string::npos);
  }
}

TEST(MarketStreamBuilder, GetPropertyInvalid) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::GET_PROPERTY);
  builder.set_combined_property(); // set combined property illegal for Get
  auto query = builder.commit();
  EXPECT_FALSE(query.has_value());
}

// Incremental request IDs
TEST(MarketStreamBuilder, RequestIdsAreUnique) {
  MarketStreamQueryBuilder builder1(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder1.add_trade_symbol("btcusdt");
  auto query1 = builder1.commit();

  MarketStreamQueryBuilder builder2(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder2.add_trade_symbol("ethusdt");
  auto query2 = builder2.commit();

  EXPECT_TRUE(query1.has_value());
  EXPECT_TRUE(query2.has_value());

  // IDs should be different
  if (query1 && query2) {
    std::string json1 = query1->str();
    std::string json2 = query2->str();
    EXPECT_NE(json1, json2); // At least IDs should differ
  }
}

// Method changing mid-build
TEST(MarketStreamBuilder, ChangeMethodMidBuild) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("btcusdt");
  builder.setMethod(MARKET_STREAM_METHOD::UNSUBSCRIBE);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("UNSUBSCRIBE"), std::string::npos);
  }
}

// Fluent interface chaining
TEST(MarketStreamBuilder, FluentInterfaceChaining) {
  auto query = MarketStreamQueryBuilder(MARKET_STREAM_METHOD::SUBSCRIBE)
                   .add_trade_symbol("btcusdt")
                   .add_aggTrade_symbol("ethusdt")
                   .add_markPrice_symbol("solusdt", true)
                   .commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("btcusdt@trade"), std::string::npos);
    EXPECT_NE(json_str.find("ethusdt@aggTrade"), std::string::npos);
    EXPECT_NE(json_str.find("solusdt@markPrice"), std::string::npos);
  }
}

// Empty subscriptions (edge case)
TEST(MarketStreamBuilder, SubscribeWithNoSymbols) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  auto query = builder.commit();

  // Shouldn't create a query with no symbols
  EXPECT_FALSE(query.has_value());
}

// Case sensitivity of symbols
TEST(MarketStreamBuilder, SymbolCasePreservation) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("BTCUSDT"); // uppercase
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    // Check if case is preserved
    EXPECT_NE(json_str.find("BTCUSDT@trade"), std::string::npos);
  }
}

// Multiple commits from same builder
TEST(MarketStreamBuilder, MultipleCommitsInvalid) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("btcusdt");

  auto query1 = builder.commit();
  auto query2 = builder.commit();

  // After query commited, the internal JSON query is cleared
  // To be able to build multiple times using one Builder instance
  EXPECT_TRUE(query1.has_value());
  EXPECT_FALSE(query2.has_value());
}

TEST(MarketStreamBuilder, MultipleCommits) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("btcusdt");

  auto query1 = builder.commit();

  builder.setMethod(MARKET_STREAM_METHOD::UNSUBSCRIBE)
      .add_bookTicker_symbol("solusdt");
  auto query2 = builder.commit();

  EXPECT_TRUE(query1.has_value());
  EXPECT_TRUE(query2.has_value());
}

// Static validation method
TEST(MarketStreamBuilder, QueryValidation) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_trade_symbol("btcusdt");
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    EXPECT_TRUE(MarketStreamQueryBuilder::is_query_valid(*query));
  }
}

// Different depth levels combinations
TEST(MarketStreamBuilder, MultipleiPartDepthLevels) {
  MarketStreamQueryBuilder builder(MARKET_STREAM_METHOD::SUBSCRIBE);
  builder.add_partDepth_symbol(
      "btcusdt", MarketStreamQueryBuilder::DEPTH_LEVELS::SMALL, true);
  builder.add_partDepth_symbol(
      "ethusdt", MarketStreamQueryBuilder::DEPTH_LEVELS::MEDIUM, true);
  builder.add_partDepth_symbol(
      "solusdt", MarketStreamQueryBuilder::DEPTH_LEVELS::LARGE, false);

  auto query = builder.commit();
  EXPECT_TRUE(query.has_value());

  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("btcusdt"), std::string::npos);
    EXPECT_NE(json_str.find("ethusdt"), std::string::npos);
    EXPECT_NE(json_str.find("solusdt"), std::string::npos);
  }
}
