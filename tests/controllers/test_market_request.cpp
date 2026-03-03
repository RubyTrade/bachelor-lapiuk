#include <gtest/gtest.h>

#include "core/controllers/market_data_utils.hpp"
#include "core/utils/constants.hpp"

// MarketRequest Tests
TEST(MarketRequest, ConstructorSetsSymbolAndType) {
    Market::MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    
    EXPECT_EQ(req.symbol, "BTCUSDT");
    EXPECT_EQ(req.type, MARKET_DATA_TYPE::TRADE);
}

TEST(MarketRequest, DefaultFastUpdateIsTrue) {
    Market::MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    EXPECT_TRUE(req.fast_update);
}

TEST(MarketRequest, DefaultDepthLevelIsSmall) {
    Market::MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::DIFF_DEPTH);
    EXPECT_EQ(req.depth_levels, MarketStreamQueryBuilder::DEPTH_LEVELS::SMALL);
}

TEST(MarketRequest, EqualityIgnoresFastUpdateAndDepthLevels) {
    Market::MarketRequest a("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    a.fast_update = true;
    a.depth_levels = MarketStreamQueryBuilder::DEPTH_LEVELS::SMALL;

    Market::MarketRequest b("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    b.fast_update = false;
    b.depth_levels = MarketStreamQueryBuilder::DEPTH_LEVELS::MEDIUM;

    EXPECT_TRUE(a == b);
}

TEST(MarketRequest, EqualityDiffersBySymbol) {
    Market::MarketRequest a("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    Market::MarketRequest b("ETHUSDT", MARKET_DATA_TYPE::TRADE);

    EXPECT_FALSE(a == b);
}

TEST(MarketRequest, EqualityDiffersByType) {
    Market::MarketRequest a("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    Market::MarketRequest b("BTCUSDT", MARKET_DATA_TYPE::DIFF_DEPTH);

    EXPECT_FALSE(a == b);
}

TEST(MarketRequest, SameRequestsAreEqual) {
    Market::MarketRequest a("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    Market::MarketRequest b("BTCUSDT", MARKET_DATA_TYPE::TRADE);

    EXPECT_TRUE(a == b);
}

TEST(MarketRequest, DifferentMarketDataTypes) {
    std::vector<MARKET_DATA_TYPE> types = {
        MARKET_DATA_TYPE::TRADE,
        MARKET_DATA_TYPE::AGG_TRADE,
        MARKET_DATA_TYPE::MARK_PRICE,
        MARKET_DATA_TYPE::DIFF_DEPTH,
        MARKET_DATA_TYPE::BOOK_TICKER
    };

    for (size_t i = 0; i < types.size(); ++i) {
        for (size_t j = i + 1; j < types.size(); ++j) {
            Market::MarketRequest a("BTCUSDT", types[i]);
            Market::MarketRequest b("BTCUSDT", types[j]);
            EXPECT_FALSE(a == b);
        }
    }
}

TEST(MarketRequest, CaseSensitiveSymbol) {
    Market::MarketRequest a("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    Market::MarketRequest b("btcusdt", MARKET_DATA_TYPE::TRADE);

    EXPECT_FALSE(a == b);
}

TEST(MarketRequest, EmptySymbol) {
    Market::MarketRequest a("", MARKET_DATA_TYPE::TRADE);
    Market::MarketRequest b("", MARKET_DATA_TYPE::TRADE);

    EXPECT_TRUE(a == b);
}

TEST(MarketRequest, DepthLevelsVariations) {
    Market::MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::DIFF_DEPTH);
    
    req.depth_levels = MarketStreamQueryBuilder::DEPTH_LEVELS::SMALL;
    EXPECT_EQ(req.depth_levels, MarketStreamQueryBuilder::DEPTH_LEVELS::SMALL);
    
    req.depth_levels = MarketStreamQueryBuilder::DEPTH_LEVELS::MEDIUM;
    EXPECT_EQ(req.depth_levels, MarketStreamQueryBuilder::DEPTH_LEVELS::MEDIUM);
    
    req.depth_levels = MarketStreamQueryBuilder::DEPTH_LEVELS::LARGE;
    EXPECT_EQ(req.depth_levels, MarketStreamQueryBuilder::DEPTH_LEVELS::LARGE);
}

// Message Type Tests
TEST(MarketMessage, StreamMessageHasCorrectType) {
    JSONQuery json(std::string("{}"));
    Market::StreamMessage msg("btcusdt@trade", json);
    
    EXPECT_EQ(msg.type, Market::MESSAGE_TYPE::STREAM);
    EXPECT_EQ(msg.stream, "btcusdt@trade");
}

TEST(MarketMessage, ResultMessageDetectsSuccess) {
    Market::ResultMessage success("null", "req-123");
    
    EXPECT_EQ(success.type, Market::MESSAGE_TYPE::RESULT);
    EXPECT_TRUE(success.isSuccessResult);
    EXPECT_EQ(success.reqId, "req-123");
}

TEST(MarketMessage, ResultMessageDetectsFailure) {
    Market::ResultMessage failure("error", "req-456");
    
    EXPECT_EQ(failure.type, Market::MESSAGE_TYPE::RESULT);
    EXPECT_FALSE(failure.isSuccessResult);
    EXPECT_EQ(failure.reqId, "req-456");
}

TEST(MarketMessage, ErrorMessageContainsCodeAndMsg) {
    Market::ErrorMessage error("ERR_001", "Connection failed");
    
    EXPECT_EQ(error.type, Market::MESSAGE_TYPE::ERROR);
    EXPECT_EQ(error.errCode, "ERR_001");
    EXPECT_EQ(error.errMsg, "Connection failed");
}

TEST(MarketMessage, UnknownMessageHasCorrectType) {
    JSONQuery json(std::string("{}"));
    Market::UnknownMessage unknown(json);
    
    EXPECT_EQ(unknown.type, Market::MESSAGE_TYPE::UNKNOWN);
}
