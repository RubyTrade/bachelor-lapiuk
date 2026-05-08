#include <gtest/gtest.h>

#include "core/utils/constants.hpp"
#include "core/utils/helper_utils.hpp"

#include <string>

// Tests for enum to string conversions and vice versa

// MARKET_DATA_TYPE Tests
TEST(Constants, MarketDataTypeToString) {
    EXPECT_EQ(type_to_str(MARKET_DATA_TYPE_STR, MARKET_DATA_TYPE::TRADE), "trade");
    EXPECT_EQ(type_to_str(MARKET_DATA_TYPE_STR, MARKET_DATA_TYPE::AGG_TRADE), "aggTrade");
    EXPECT_EQ(type_to_str(MARKET_DATA_TYPE_STR, MARKET_DATA_TYPE::MARK_PRICE), "markPrice");
    EXPECT_EQ(type_to_str(MARKET_DATA_TYPE_STR, MARKET_DATA_TYPE::BOOK_TICKER), "bookTicker");
    EXPECT_EQ(type_to_str(MARKET_DATA_TYPE_STR, MARKET_DATA_TYPE::DIFF_DEPTH), "depth");
}

TEST(Constants, StringToMarketDataType) {
    EXPECT_EQ(str_to_type(MARKET_DATA_TYPE_STR, std::string("trade")), MARKET_DATA_TYPE::TRADE);
    EXPECT_EQ(str_to_type(MARKET_DATA_TYPE_STR, std::string("aggTrade")), MARKET_DATA_TYPE::AGG_TRADE);
    EXPECT_EQ(str_to_type(MARKET_DATA_TYPE_STR, std::string("markPrice")), MARKET_DATA_TYPE::MARK_PRICE);
    EXPECT_EQ(str_to_type(MARKET_DATA_TYPE_STR, std::string("bookTicker")), MARKET_DATA_TYPE::BOOK_TICKER);
}

// ORDER_TYPE Tests
TEST(Constants, OrderTypeToString) {
    EXPECT_EQ(type_to_str(ORDER_TYPE_STR, ORDER_TYPE::MARKET), "MARKET");
    EXPECT_EQ(type_to_str(ORDER_TYPE_STR, ORDER_TYPE::LIMIT), "LIMIT");
    EXPECT_EQ(type_to_str(ORDER_TYPE_STR, ORDER_TYPE::STOP), "STOP");
    EXPECT_EQ(type_to_str(ORDER_TYPE_STR, ORDER_TYPE::STOP_MARKET), "STOP_MARKET");
    EXPECT_EQ(type_to_str(ORDER_TYPE_STR, ORDER_TYPE::TAKE_PROFIT), "TAKE_PROFIT");
    EXPECT_EQ(type_to_str(ORDER_TYPE_STR, ORDER_TYPE::TAKE_PROFIT_MARKET), "TAKE_PROFIT_MARKET");
    EXPECT_EQ(type_to_str(ORDER_TYPE_STR, ORDER_TYPE::TRAILING_STOP_MARKET), "TRAILING_STOP_MARKET");
}

TEST(Constants, StringToOrderType) {
    EXPECT_EQ(str_to_type(ORDER_TYPE_STR, std::string("MARKET")), ORDER_TYPE::MARKET);
    EXPECT_EQ(str_to_type(ORDER_TYPE_STR, std::string("LIMIT")), ORDER_TYPE::LIMIT);
    EXPECT_EQ(str_to_type(ORDER_TYPE_STR, std::string("STOP_MARKET")), ORDER_TYPE::STOP_MARKET);
}

// ORDER_SIDE Tests
TEST(Constants, OrderSideToString) {
    EXPECT_EQ(type_to_str(ORDER_SIDE_STR, ORDER_SIDE::BUY), "BUY");
    EXPECT_EQ(type_to_str(ORDER_SIDE_STR, ORDER_SIDE::SELL), "SELL");
}

TEST(Constants, StringToOrderSide) {
    EXPECT_EQ(str_to_type(ORDER_SIDE_STR, std::string("BUY")), ORDER_SIDE::BUY);
    EXPECT_EQ(str_to_type(ORDER_SIDE_STR, std::string("SELL")), ORDER_SIDE::SELL);
}

// POSITION_SIDE Tests
TEST(Constants, PositionSideToString) {
    EXPECT_EQ(type_to_str(POSITION_SIDE_STR, POSITION_SIDE::BOTH), "BOTH");
    EXPECT_EQ(type_to_str(POSITION_SIDE_STR, POSITION_SIDE::LONG), "LONG");
    EXPECT_EQ(type_to_str(POSITION_SIDE_STR, POSITION_SIDE::SHORT), "SHORT");
}

TEST(Constants, StringToPositionSide) {
    EXPECT_EQ(str_to_type(POSITION_SIDE_STR, std::string("BOTH")), POSITION_SIDE::BOTH);
    EXPECT_EQ(str_to_type(POSITION_SIDE_STR, std::string("LONG")), POSITION_SIDE::LONG);
    EXPECT_EQ(str_to_type(POSITION_SIDE_STR, std::string("SHORT")), POSITION_SIDE::SHORT);
}

// TIME_IN_FORCE Tests
TEST(Constants, TimeInForceToString) {
    EXPECT_EQ(type_to_str(TIME_IN_FORCE_STR, TIME_IN_FORCE::GTC), "GTC");
    EXPECT_EQ(type_to_str(TIME_IN_FORCE_STR, TIME_IN_FORCE::IOC), "IOC");
    EXPECT_EQ(type_to_str(TIME_IN_FORCE_STR, TIME_IN_FORCE::FOK), "FOK");
    EXPECT_EQ(type_to_str(TIME_IN_FORCE_STR, TIME_IN_FORCE::GTX), "TGX");
    EXPECT_EQ(type_to_str(TIME_IN_FORCE_STR, TIME_IN_FORCE::GTD), "GTD");
    EXPECT_EQ(type_to_str(TIME_IN_FORCE_STR, TIME_IN_FORCE::RPI), "RPI");
}

TEST(Constants, StringToTimeInForce) {
    EXPECT_EQ(str_to_type(TIME_IN_FORCE_STR, std::string("GTC")), TIME_IN_FORCE::GTC);
    EXPECT_EQ(str_to_type(TIME_IN_FORCE_STR, std::string("IOC")), TIME_IN_FORCE::IOC);
    EXPECT_EQ(str_to_type(TIME_IN_FORCE_STR, std::string("FOK")), TIME_IN_FORCE::FOK);
}

// USER_DATA_EVENT_TYPE Tests
TEST(Constants, UserDataEventTypeToString) {
    EXPECT_EQ(type_to_str(USER_DATA_EVENT_TYPE_STR, USER_DATA_EVENT_TYPE::LISTEN_KEY_EXPIRY), "listenKeyExpired");
    EXPECT_EQ(type_to_str(USER_DATA_EVENT_TYPE_STR, USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE), "ACCOUNT_UPDATE");
    EXPECT_EQ(type_to_str(USER_DATA_EVENT_TYPE_STR, USER_DATA_EVENT_TYPE::MARGIN_CALL), "MARGIN_CALL");
    EXPECT_EQ(type_to_str(USER_DATA_EVENT_TYPE_STR, USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE), "ORDER_TRADE_UPDATE");
    EXPECT_EQ(type_to_str(USER_DATA_EVENT_TYPE_STR, USER_DATA_EVENT_TYPE::TRADE_LITE), "TRADE_LITE");
    EXPECT_EQ(type_to_str(USER_DATA_EVENT_TYPE_STR, USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE), "ACCOUNT_CONFIG_UPDATE");
    EXPECT_EQ(type_to_str(USER_DATA_EVENT_TYPE_STR, USER_DATA_EVENT_TYPE::ALGO_UPDATE), "ALGO_UPDATE");
}

TEST(Constants, StringToUserDataEventType) {
    EXPECT_EQ(str_to_type(USER_DATA_EVENT_TYPE_STR, std::string("ACCOUNT_UPDATE")), USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE);
    EXPECT_EQ(str_to_type(USER_DATA_EVENT_TYPE_STR, std::string("MARGIN_CALL")), USER_DATA_EVENT_TYPE::MARGIN_CALL);
    EXPECT_EQ(str_to_type(USER_DATA_EVENT_TYPE_STR, std::string("ORDER_TRADE_UPDATE")), USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE);
    EXPECT_EQ(str_to_type(USER_DATA_EVENT_TYPE_STR, std::string("ALGO_UPDATE")), USER_DATA_EVENT_TYPE::ALGO_UPDATE);
}

// EXECUTION_TYPE Tests
TEST(Constants, ExecutionTypeToString) {
    EXPECT_EQ(type_to_str(EXECUTION_TYPE_STR, EXECUTION_TYPE::NEW), "NEW");
    EXPECT_EQ(type_to_str(EXECUTION_TYPE_STR, EXECUTION_TYPE::CANCELED), "CANCELED");
    EXPECT_EQ(type_to_str(EXECUTION_TYPE_STR, EXECUTION_TYPE::CALCULATED), "CALCULATED");
    EXPECT_EQ(type_to_str(EXECUTION_TYPE_STR, EXECUTION_TYPE::EXPIRED), "EXPIRED");
    EXPECT_EQ(type_to_str(EXECUTION_TYPE_STR, EXECUTION_TYPE::TRADE), "TRADE");
}

TEST(Constants, StringToExecutionType) {
    EXPECT_EQ(str_to_type(EXECUTION_TYPE_STR, std::string("NEW")), EXECUTION_TYPE::NEW);
    EXPECT_EQ(str_to_type(EXECUTION_TYPE_STR, std::string("TRADE")), EXECUTION_TYPE::TRADE);
    EXPECT_EQ(str_to_type(EXECUTION_TYPE_STR, std::string("CANCELED")), EXECUTION_TYPE::CANCELED);
}

// ORDER_STATUS Tests
TEST(Constants, OrderStatusToString) {
    EXPECT_EQ(type_to_str(ORDER_STATUS_STR, ORDER_STATUS::NEW), "NEW");
    EXPECT_EQ(type_to_str(ORDER_STATUS_STR, ORDER_STATUS::PARTIALLY_FILLED), "PARTIALLY_FILLED");
    EXPECT_EQ(type_to_str(ORDER_STATUS_STR, ORDER_STATUS::FILLED), "FILLED");
    EXPECT_EQ(type_to_str(ORDER_STATUS_STR, ORDER_STATUS::CANCELED), "CANCELED");
    EXPECT_EQ(type_to_str(ORDER_STATUS_STR, ORDER_STATUS::EXPIRED), "EXPIRED");
    EXPECT_EQ(type_to_str(ORDER_STATUS_STR, ORDER_STATUS::REJECTED), "REJECTED");
}

TEST(Constants, StringToOrderStatus) {
    EXPECT_EQ(str_to_type(ORDER_STATUS_STR, std::string("NEW")), ORDER_STATUS::NEW);
    EXPECT_EQ(str_to_type(ORDER_STATUS_STR, std::string("FILLED")), ORDER_STATUS::FILLED);
    EXPECT_EQ(str_to_type(ORDER_STATUS_STR, std::string("PARTIALLY_FILLED")), ORDER_STATUS::PARTIALLY_FILLED);
}

// WORKING_TYPE Tests
TEST(Constants, WorkingTypeToString) {
    EXPECT_EQ(type_to_str(WORKING_TYPE_STR, WORKING_TYPE::CONTRACT_PRICE), "CONTRACT_PRICE");
    EXPECT_EQ(type_to_str(WORKING_TYPE_STR, WORKING_TYPE::MARK_PRICE), "MARK_PRICE");
}

TEST(Constants, StringToWorkingType) {
    EXPECT_EQ(str_to_type(WORKING_TYPE_STR, std::string("CONTRACT_PRICE")), WORKING_TYPE::CONTRACT_PRICE);
    EXPECT_EQ(str_to_type(WORKING_TYPE_STR, std::string("MARK_PRICE")), WORKING_TYPE::MARK_PRICE);
}

// SELF_TRADE_PREVENTION_MODE Tests
TEST(Constants, SelfTradePreventionModeToString) {
    EXPECT_EQ(type_to_str(SELF_TRADE_PREVENTION_MODE_STR, SELF_TRADE_PREVENTION_MODE::NONE), "NONE");
    EXPECT_EQ(type_to_str(SELF_TRADE_PREVENTION_MODE_STR, SELF_TRADE_PREVENTION_MODE::EXPIRE_MAKER), "EXPIRE_MAKER");
    EXPECT_EQ(type_to_str(SELF_TRADE_PREVENTION_MODE_STR, SELF_TRADE_PREVENTION_MODE::EXPIRE_TAKER), "EXPIRE_TAKER");
    EXPECT_EQ(type_to_str(SELF_TRADE_PREVENTION_MODE_STR, SELF_TRADE_PREVENTION_MODE::EXPIRE_BOTH), "EXPIRE_BOTH");
}

TEST(Constants, StringToSelfTradePreventionMode) {
    EXPECT_EQ(str_to_type(SELF_TRADE_PREVENTION_MODE_STR, std::string("NONE")), SELF_TRADE_PREVENTION_MODE::NONE);
    EXPECT_EQ(str_to_type(SELF_TRADE_PREVENTION_MODE_STR, std::string("EXPIRE_MAKER")), SELF_TRADE_PREVENTION_MODE::EXPIRE_MAKER);
}

// Round-trip conversion tests
TEST(Constants, OrderTypeRoundTrip) {
    std::vector<ORDER_TYPE> types = {
        ORDER_TYPE::MARKET, ORDER_TYPE::LIMIT, ORDER_TYPE::STOP,
        ORDER_TYPE::STOP_MARKET, ORDER_TYPE::TAKE_PROFIT,
        ORDER_TYPE::TAKE_PROFIT_MARKET, ORDER_TYPE::TRAILING_STOP_MARKET
    };
    
    for (const auto& type : types) {
        std::string str = type_to_str(ORDER_TYPE_STR, type);
        ORDER_TYPE converted = str_to_type(ORDER_TYPE_STR, str);
        EXPECT_EQ(converted, type);
    }
}

TEST(Constants, OrderStatusRoundTrip) {
    std::vector<ORDER_STATUS> statuses = {
        ORDER_STATUS::NEW, ORDER_STATUS::PARTIALLY_FILLED,
        ORDER_STATUS::FILLED, ORDER_STATUS::CANCELED,
        ORDER_STATUS::EXPIRED, ORDER_STATUS::REJECTED
    };
    
    for (const auto& status : statuses) {
        std::string str = type_to_str(ORDER_STATUS_STR, status);
        ORDER_STATUS converted = str_to_type(ORDER_STATUS_STR, str);
        EXPECT_EQ(converted, status);
    }
}

TEST(Constants, PositionSideRoundTrip) {
    std::vector<POSITION_SIDE> sides = {
        POSITION_SIDE::BOTH, POSITION_SIDE::LONG, POSITION_SIDE::SHORT
    };
    
    for (const auto& side : sides) {
        std::string str = type_to_str(POSITION_SIDE_STR, side);
        POSITION_SIDE converted = str_to_type(POSITION_SIDE_STR, str);
        EXPECT_EQ(converted, side);
    }
}

// Invalid/Unknown string tests
TEST(Constants, UnknownStringReturnsFirstValue) {
    // When an unknown string is passed, should return first value in enum array
    auto result = str_to_type(ORDER_SIDE_STR, std::string("UNKNOWN"));
    // First element is BUY
    EXPECT_EQ(result, ORDER_SIDE::BUY);
}

TEST(Constants, EmptyStringHandling) {
    auto result = str_to_type(ORDER_TYPE_STR, std::string(""));
    // Should return first element (MARKET)
    EXPECT_EQ(result, ORDER_TYPE::MARKET);
}

// Case sensitivity tests
TEST(Constants, CaseSensitiveMatching) {
    // These should NOT match (case sensitive)
    auto result1 = str_to_type(ORDER_SIDE_STR, std::string("buy"));
    auto result2 = str_to_type(ORDER_SIDE_STR, std::string("Buy"));
    
    // Should return default (first element)
    EXPECT_EQ(result1, ORDER_SIDE::BUY);
    EXPECT_EQ(result2, ORDER_SIDE::BUY);
}

// Data namespace constants tests
TEST(Constants, BinanceConstants) {
    EXPECT_EQ(Data::WS_HOST, "fstream.binance.com");
    EXPECT_EQ(Data::WS_TRADING_HOST, "ws-fapi.binance.com");
    EXPECT_EQ(Data::API_HOST, "fapi.binance.com");
    EXPECT_EQ(Data::WS_PORT_MAIN, 443);
    EXPECT_EQ(Data::HTTPS_PORT, 443);
}

TEST(Constants, HttpHeaderConstants) {
    EXPECT_EQ(Data::Header::APIKEY, "X-MBX-APIKEY");
    EXPECT_EQ(Data::Header::CONTENT_TYPE, "Content-Type");
    EXPECT_EQ(Data::Header::CONTENT_LENGTH, "Content-Length");
    EXPECT_EQ(Data::Header::CONNECTION, "Connection");
}

TEST(Constants, EnvironmentVariableNames) {
    EXPECT_EQ(Data::BINANCE_READ_APIKEY_ENV, "BINANCE_READ_API_KEY");
    EXPECT_EQ(Data::BINANCE_WRITE_APIKEY_ENV, "BINANCE_WRITE_API_KEY");
    EXPECT_EQ(Data::BINANCE_PK_ENV, "BINANCE_PRIVATE_KEY");
}
