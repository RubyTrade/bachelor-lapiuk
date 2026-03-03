#include <gtest/gtest.h>

#include "core/stream/trading_stream.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"

#include <string>

// Tests for ParametersBuilder

// Basic parameter additions
TEST(ParametersBuilder, AddSymbol) {
  ParametersBuilder builder;
  builder.add_symbol("BTCUSDT");
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("BTCUSDT"), std::string::npos);
    EXPECT_NE(json_str.find("symbol"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddPositionSide) {
  ParametersBuilder builder;
  builder.add_positionSide(POSITION_SIDE::LONG);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("LONG"), std::string::npos);
    EXPECT_NE(json_str.find("positionSide"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddPrice) {
  ParametersBuilder builder;
  builder.add_price(Fixed("42000.50"));
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("price"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddQuantity) {
  ParametersBuilder builder;
  builder.add_quantity(Fixed("0.5"));
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("quantity"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddOrderSide) {
  ParametersBuilder builder;
  builder.add_side(ORDER_SIDE::BUY);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("BUY"), std::string::npos);
    EXPECT_NE(json_str.find("side"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddTimeInForce) {
  ParametersBuilder builder;
  builder.add_timeInForce(TIME_IN_FORCE::GTC);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("GTC"), std::string::npos);
    EXPECT_NE(json_str.find("timeInForce"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddOrderType) {
  ParametersBuilder builder;
  builder.add_type(ORDER_TYPE::LIMIT);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("LIMIT"), std::string::npos);
    EXPECT_NE(json_str.find("type"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddClientOrderId) {
  ParametersBuilder builder;
  builder.add_clientOrderId("my_order_123");
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("my_order_123"), std::string::npos);
    EXPECT_NE(json_str.find("newClientOrderId"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddReduceOnly) {
  ParametersBuilder builder;
  builder.add_reduceOnly(true);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("reduceOnly"), std::string::npos);
  }
}

// Algo parameters
TEST(ParametersBuilder, AddAlgoType) {
  ParametersBuilder builder;
  builder.add_algoType(ORDER_TYPE::TRAILING_STOP_MARKET);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("TRAILING_STOP_MARKET"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddNewOrderRespType) {
  ParametersBuilder builder;
  builder.add_newOrderRespType("RESULT");
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("RESULT"), std::string::npos);
    EXPECT_NE(json_str.find("newOrderRespType"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddTriggerPrice) {
  ParametersBuilder builder;
  builder.add_triggerPrice(Fixed("43000.00"));
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("triggerPrice"), std::string::npos);
  }
}

// Additional parameters
TEST(ParametersBuilder, AddOrderId) {
  ParametersBuilder builder;
  builder.add_orderId(123456789);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("123456789"), std::string::npos);
    EXPECT_NE(json_str.find("orderId"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddOrigClientOrderId) {
  ParametersBuilder builder;
  builder.add_origClientOrderId("original_order_456");
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("original_order_456"), std::string::npos);
    EXPECT_NE(json_str.find("origClientOrderId"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddPriceMatch) {
  ParametersBuilder builder;
  builder.add_priceMatch("OPPONENT");
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("OPPONENT"), std::string::npos);
    EXPECT_NE(json_str.find("priceMatch"), std::string::npos);
  }
}

TEST(ParametersBuilder, AddOrigType) {
  ParametersBuilder builder;
  builder.add_origType(ORDER_TYPE::STOP_MARKET);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("STOP_MARKET"), std::string::npos);
  }
}

// Complete order parameters
TEST(ParametersBuilder, CompleteLimitOrder) {
  ParametersBuilder builder;
  auto query = builder.add_symbol("BTCUSDT")
                   .add_side(ORDER_SIDE::BUY)
                   .add_positionSide(POSITION_SIDE::LONG)
                   .add_type(ORDER_TYPE::LIMIT)
                   .add_quantity(Fixed("0.1"))
                   .add_price(Fixed("42000.50"))
                   .add_timeInForce(TIME_IN_FORCE::GTC)
                   .commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("BTCUSDT"), std::string::npos);
    EXPECT_NE(json_str.find("BUY"), std::string::npos);
    EXPECT_NE(json_str.find("LONG"), std::string::npos);
    EXPECT_NE(json_str.find("LIMIT"), std::string::npos);
    EXPECT_NE(json_str.find("GTC"), std::string::npos);
  }
}

TEST(ParametersBuilder, CompleteMarketOrder) {
  ParametersBuilder builder;
  auto query = builder.add_symbol("ETHUSDT")
                   .add_side(ORDER_SIDE::SELL)
                   .add_positionSide(POSITION_SIDE::SHORT)
                   .add_type(ORDER_TYPE::MARKET)
                   .add_quantity(Fixed("1.5"))
                   .commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("ETHUSDT"), std::string::npos);
    EXPECT_NE(json_str.find("SELL"), std::string::npos);
    EXPECT_NE(json_str.find("SHORT"), std::string::npos);
    EXPECT_NE(json_str.find("MARKET"), std::string::npos);
  }
}

TEST(ParametersBuilder, StopLossOrder) {
  ParametersBuilder builder;
  auto query = builder.add_symbol("BTCUSDT")
                   .add_side(ORDER_SIDE::SELL)
                   .add_type(ORDER_TYPE::STOP_MARKET)
                   .add_quantity(Fixed("0.05"))
                   .add_triggerPrice(Fixed("40000.00"))
                   .add_reduceOnly(true)
                   .commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("STOP_MARKET"), std::string::npos);
    EXPECT_NE(json_str.find("triggerPrice"), std::string::npos);
    EXPECT_NE(json_str.find("reduceOnly"), std::string::npos);
  }
}

TEST(ParametersBuilder, TakeProfitOrder) {
  ParametersBuilder builder;
  auto query = builder.add_symbol("ETHUSDT")
                   .add_side(ORDER_SIDE::SELL)
                   .add_type(ORDER_TYPE::TAKE_PROFIT_MARKET)
                   .add_quantity(Fixed("2.0"))
                   .add_triggerPrice(Fixed("3500.00"))
                   .commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("TAKE_PROFIT_MARKET"), std::string::npos);
  }
}

// Order modification parameters
TEST(ParametersBuilder, ModifyOrderParameters) {
  ParametersBuilder builder;
  auto query = builder.add_symbol("BTCUSDT")
                   .add_orderId(987654321)
                   .add_quantity(Fixed("0.2"))
                   .add_price(Fixed("43500.00"))
                   .commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("987654321"), std::string::npos);
  }
}

TEST(ParametersBuilder, CancelOrderByClientId) {
  ParametersBuilder builder;
  auto query = builder.add_symbol("BTCUSDT")
                   .add_origClientOrderId("order_to_cancel_789")
                   .commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("order_to_cancel_789"), std::string::npos);
  }
}

// Different position sides
TEST(ParametersBuilder, PositionSideBoth) {
  ParametersBuilder builder;
  builder.add_positionSide(POSITION_SIDE::BOTH);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("BOTH"), std::string::npos);
  }
}

// Different time in force values
TEST(ParametersBuilder, TimeInForceIOC) {
  ParametersBuilder builder;
  builder.add_timeInForce(TIME_IN_FORCE::IOC);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("IOC"), std::string::npos);
  }
}

TEST(ParametersBuilder, TimeInForceFOK) {
  ParametersBuilder builder;
  builder.add_timeInForce(TIME_IN_FORCE::FOK);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  if (query) {
    std::string json_str = query->str();
    EXPECT_NE(json_str.find("FOK"), std::string::npos);
  }
}

// Precision tests with Fixed numbers
TEST(ParametersBuilder, HighPrecisionPrice) {
  ParametersBuilder builder;
  builder.add_price(Fixed("42123.456789"));
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
}

TEST(ParametersBuilder, SmallQuantity) {
  ParametersBuilder builder;
  builder.add_quantity(Fixed("0.00001"));
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
}

// Empty builder
TEST(ParametersBuilder, EmptyBuilder) {
  ParametersBuilder builder;
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
  // Should return valid but empty parameters
}

// Multiple commits
TEST(ParametersBuilder, MultipleCommitsFromSameBuilder) {
  ParametersBuilder builder;
  builder.add_symbol("BTCUSDT");

  auto query1 = builder.commit();
  auto query2 = builder.commit();

  EXPECT_TRUE(query1.has_value());
  EXPECT_TRUE(query2.has_value());
}

// Fluent interface chaining
TEST(ParametersBuilder, FluentInterfaceChaining) {
  auto query = ParametersBuilder()
                   .add_symbol("BTCUSDT")
                   .add_side(ORDER_SIDE::BUY)
                   .add_type(ORDER_TYPE::LIMIT)
                   .add_quantity(Fixed("0.5"))
                   .add_price(Fixed("42000.00"))
                   .add_timeInForce(TIME_IN_FORCE::GTC)
                   .add_clientOrderId("my_limit_order")
                   .commit();

  EXPECT_TRUE(query.has_value());
}

// Edge cases
TEST(ParametersBuilder, ZeroQuantity) {
  ParametersBuilder builder;
  builder.add_quantity(Fixed("0"));
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
}

TEST(ParametersBuilder, VeryLargeOrderId) {
  ParametersBuilder builder;
  builder.add_orderId(9223372036854775807LL); // max int64_t
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
}

TEST(ParametersBuilder, EmptyClientOrderId) {
  ParametersBuilder builder;
  builder.add_clientOrderId("");
  auto query = builder.commit();

  EXPECT_FALSE(query.has_value());
}

// ReduceOnly flag variations
TEST(ParametersBuilder, ReduceOnlyFalse) {
  ParametersBuilder builder;
  builder.add_reduceOnly(false);
  auto query = builder.commit();

  EXPECT_TRUE(query.has_value());
}
