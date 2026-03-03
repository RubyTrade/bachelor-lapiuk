#include <gtest/gtest.h>

#include "core/account_manager/order_book.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/fixed_num.hpp"

#include <chrono>
#include <thread>

using namespace UserData;

// Helper functions to create test events
OrderTradeUpdateEvent createOrderTradeUpdate(
    const std::string &symbol, const std::string &clientOrderId,
    uint64_t orderId = 12345, ORDER_SIDE side = ORDER_SIDE::BUY,
    ORDER_TYPE orderType = ORDER_TYPE::LIMIT,
    ORDER_STATUS status = ORDER_STATUS::NEW) {

  OrderTradeUpdateEvent event;
  event.symbol = symbol;
  event.clientOrderId = clientOrderId;
  event.orderId = orderId;
  event.side = side;
  event.orderType = orderType;
  event.status = status;
  event.timeInForce = TIME_IN_FORCE::GTC;
  event.executionType = EXECUTION_TYPE::NEW;
  event.positionSide = POSITION_SIDE::BOTH;
  event.origQty = Fixed("1.0");
  event.executedQty = Fixed("0.0");
  event.lastExecutedQty = Fixed("0.0");
  event.price = Fixed("50000.0");
  event.lastPrice = Fixed("0.0");
  event.avgPrice = Fixed("0.0");
  event.commission = Fixed("0.0");
  event.commissionAsset = "USDT";
  event.eventTime = 1234567890;
  event.tradeTime = 1234567890;
  event.realizedPnL = Fixed("0.0");
  event.reduceOnly = false;

  return event;
}

TradeLiteEvent createTradeLite(const std::string &symbol,
                               const std::string &clientOrderId,
                               uint64_t orderId = 12345,
                               ORDER_SIDE side = ORDER_SIDE::BUY) {

  TradeLiteEvent event;
  event.symbol = symbol;
  event.clientOrderId = clientOrderId;
  event.orderId = orderId;
  event.side = side;
  event.price = Fixed("50000.0");
  event.qty = Fixed("1.0");
  event.lastPrice = Fixed("50000.0");
  event.lastQty = Fixed("1.0");
  event.eventTime = 1234567890;
  event.tradeTime = 1234567890;

  return event;
}

// AccountOrders Tests
TEST(AccountOrders, EmptyByDefault) {
  AccountOrders orders;
  auto order = orders.getOrderByClientId("non-existent");
  EXPECT_FALSE(order.has_value());
}

TEST(AccountOrders, InsertAndRetrieve) {
  AccountOrders orders;

  orders.tryEmplace("client-1", [](OrderEntry &entry) {
    entry.clientOrderId = "client-1";
    entry.symbol = "BTCUSDT";
    entry.orderId = 123;
    entry.side = ORDER_SIDE::BUY;
    entry.price = Fixed("50000");
  });

  auto retrieved = orders.getOrderByClientId("client-1");
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ(retrieved->clientOrderId, "client-1");
  EXPECT_EQ(retrieved->symbol, "BTCUSDT");
  EXPECT_EQ(retrieved->orderId, 123u);
  EXPECT_EQ(retrieved->side, ORDER_SIDE::BUY);
}

TEST(AccountOrders, UpdateExistingOrder) {
  AccountOrders orders;

  orders.tryEmplace("client-1", [](OrderEntry &entry) {
    entry.clientOrderId = "client-1";
    entry.orderStatus = ORDER_STATUS::NEW;
    entry.executedQty = Fixed("0.0");
  });

  orders.tryEmplace("client-1", [](OrderEntry &entry) {
    entry.orderStatus = ORDER_STATUS::PARTIALLY_FILLED;
    entry.executedQty = Fixed("0.5");
  });

  auto retrieved = orders.getOrderByClientId("client-1");
  ASSERT_TRUE(retrieved.has_value());
  EXPECT_EQ(retrieved->orderStatus, ORDER_STATUS::PARTIALLY_FILLED);
  EXPECT_EQ(retrieved->executedQty.to_string(), "0.5");
}

TEST(AccountOrders, RemoveOrder) {
  AccountOrders orders;

  orders.tryEmplace(
      "client-1", [](OrderEntry &entry) { entry.clientOrderId = "client-1"; });

  EXPECT_TRUE(orders.getOrderByClientId("client-1").has_value());

  orders.tryRemove("client-1");
  EXPECT_FALSE(orders.getOrderByClientId("client-1").has_value());
}

TEST(AccountOrders, RemoveNonExistentOrder) {
  AccountOrders orders;

  // Should not crash
  orders.tryRemove("non-existent");
  EXPECT_FALSE(orders.getOrderByClientId("non-existent").has_value());
}

TEST(AccountOrders, MultipleOrders) {
  AccountOrders orders;

  for (int i = 0; i < 10; ++i) {
    std::string clientId = "client-" + std::to_string(i);
    orders.tryEmplace(clientId, [clientId, i](OrderEntry &entry) {
      entry.clientOrderId = clientId;
      entry.orderId = i;
    });
  }

  for (int i = 0; i < 10; ++i) {
    std::string clientId = "client-" + std::to_string(i);
    auto order = orders.getOrderByClientId(clientId);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->orderId, static_cast<uint64_t>(i));
  }
}

TEST(AccountOrders, ThreadSafeInsert) {
  AccountOrders orders;
  const int numThreads = 5;
  const int ordersPerThread = 100;

  std::vector<std::thread> threads;
  for (int t = 0; t < numThreads; ++t) {
    threads.emplace_back([&orders, t, ordersPerThread]() {
      for (int i = 0; i < ordersPerThread; ++i) {
        std::string clientId =
            "thread-" + std::to_string(t) + "-order-" + std::to_string(i);
        orders.tryEmplace(clientId, [clientId](OrderEntry &entry) {
          entry.clientOrderId = clientId;
        });
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  // Verify all orders were inserted
  for (int t = 0; t < numThreads; ++t) {
    for (int i = 0; i < ordersPerThread; ++i) {
      std::string clientId =
          "thread-" + std::to_string(t) + "-order-" + std::to_string(i);
      EXPECT_TRUE(orders.getOrderByClientId(clientId).has_value());
    }
  }
}

// OrderBook Tests
TEST(OrderBook, InitializedWithZeroUpdateTime) {
  OrderBook orderBook;
  EXPECT_GT(orderBook.getLastUpdateTime(), 0u);
}

TEST(OrderBook, EnqueueAndProcessOrderTradeUpdate) {
  OrderBook orderBook;

  orderBook.start();

  auto event = createOrderTradeUpdate("BTCUSDT", "client-1");
  ParsedUserData parsedEvent = event;

  orderBook.enqueue(std::move(parsedEvent));

  // Give some time for processing
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto order = orderBook.getOrderByClientId("client-1");
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->symbol, "BTCUSDT");
  EXPECT_EQ(order->clientOrderId, "client-1");
  EXPECT_EQ(order->fulfilledBy, USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE);
}

TEST(OrderBook, EnqueueAndProcessTradeLite) {
  OrderBook orderBook;

  orderBook.start();

  auto event = createTradeLite("ETHUSDT", "client-2");
  ParsedUserData parsedEvent = event;

  orderBook.enqueue(std::move(parsedEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto order = orderBook.getOrderByClientId("client-2");
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->symbol, "ETHUSDT");
  EXPECT_EQ(order->clientOrderId, "client-2");
  EXPECT_EQ(order->fulfilledBy, USER_DATA_EVENT_TYPE::TRADE_LITE);
}

TEST(OrderBook, TradeLiteDoesNotOverrideOrderTradeUpdate) {
  OrderBook orderBook;

  orderBook.start();

  // First send OrderTradeUpdate
  auto orderEvent = createOrderTradeUpdate("BTCUSDT", "client-1");
  orderEvent.avgPrice = Fixed("50500.0");
  ParsedUserData parsedOrderEvent = orderEvent;
  orderBook.enqueue(std::move(parsedOrderEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Then send TradeLite for the same order
  auto tradeEvent = createTradeLite("BTCUSDT", "client-1");
  ParsedUserData parsedTradeEvent = tradeEvent;
  orderBook.enqueue(std::move(parsedTradeEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto order = orderBook.getOrderByClientId("client-1");
  ASSERT_TRUE(order.has_value());
  // Should still be fulfilled by ORDER_TRADE_UPDATE
  EXPECT_EQ(order->fulfilledBy, USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE);
  // TradeLite should not have overridden the avgPrice
  EXPECT_EQ(order->avgPrice.to_string(), "50500.0");
}

TEST(OrderBook, OrderTradeUpdateOverridesTradeLite) {
  OrderBook orderBook;

  orderBook.start();

  // First send TradeLite
  auto tradeEvent = createTradeLite("BTCUSDT", "client-1");
  ParsedUserData parsedTradeEvent = tradeEvent;
  orderBook.enqueue(std::move(parsedTradeEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Then send OrderTradeUpdate for the same order
  auto orderEvent = createOrderTradeUpdate("BTCUSDT", "client-1");
  orderEvent.avgPrice = Fixed("50500.0");
  ParsedUserData parsedOrderEvent = orderEvent;
  orderBook.enqueue(std::move(parsedOrderEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto order = orderBook.getOrderByClientId("client-1");
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->fulfilledBy, USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE);
  EXPECT_EQ(order->avgPrice.to_string(), "50500.0");
}

TEST(OrderBook, UpdateTimeChangesAfterEvent) {
  OrderBook orderBook;

  orderBook.start();

  uint64_t initialTime = orderBook.getLastUpdateTime();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto event = createOrderTradeUpdate("BTCUSDT", "client-1");
  ParsedUserData parsedEvent = event;
  orderBook.enqueue(std::move(parsedEvent));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  uint64_t afterEventTime = orderBook.getLastUpdateTime();
  EXPECT_GT(afterEventTime, initialTime);
}

TEST(OrderBook, MultipleOrdersDifferentSymbols) {
  OrderBook orderBook;

  orderBook.start();

  std::vector<std::string> symbols = {"BTCUSDT", "ETHUSDT", "BNBUSDT",
                                      "ADAUSDT"};

  for (size_t i = 0; i < symbols.size(); ++i) {
    std::string clientId = "client-" + std::to_string(i);
    auto event = createOrderTradeUpdate(symbols[i], clientId);
    ParsedUserData parsedEvent = event;
    orderBook.enqueue(std::move(parsedEvent));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (size_t i = 0; i < symbols.size(); ++i) {
    std::string clientId = "client-" + std::to_string(i);
    auto order = orderBook.getOrderByClientId(clientId);
    ASSERT_TRUE(order.has_value());
    EXPECT_EQ(order->symbol, symbols[i]);
  }
}

TEST(OrderBook, OrderStatusTransitions) {
  OrderBook orderBook;

  orderBook.start();

  // NEW order
  auto event1 =
      createOrderTradeUpdate("BTCUSDT", "client-1", 123, ORDER_SIDE::BUY,
                             ORDER_TYPE::LIMIT, ORDER_STATUS::NEW);
  ParsedUserData parsed1 = event1;
  orderBook.enqueue(std::move(parsed1));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto order = orderBook.getOrderByClientId("client-1");
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->orderStatus, ORDER_STATUS::NEW);

  // PARTIALLY_FILLED
  auto event2 =
      createOrderTradeUpdate("BTCUSDT", "client-1", 123, ORDER_SIDE::BUY,
                             ORDER_TYPE::LIMIT, ORDER_STATUS::PARTIALLY_FILLED);
  event2.executedQty = Fixed("0.5");
  ParsedUserData parsed2 = event2;
  orderBook.enqueue(std::move(parsed2));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  order = orderBook.getOrderByClientId("client-1");
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->orderStatus, ORDER_STATUS::PARTIALLY_FILLED);

  // FILLED
  auto event3 =
      createOrderTradeUpdate("BTCUSDT", "client-1", 123, ORDER_SIDE::BUY,
                             ORDER_TYPE::LIMIT, ORDER_STATUS::FILLED);
  event3.executedQty = Fixed("1.0");
  ParsedUserData parsed3 = event3;
  orderBook.enqueue(std::move(parsed3));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  order = orderBook.getOrderByClientId("client-1");
  ASSERT_TRUE(order.has_value());
  EXPECT_EQ(order->orderStatus, ORDER_STATUS::FILLED);
  EXPECT_EQ(order->executedQty.to_string(), "1.0");
}

TEST(OrderBook, ErrorParseIsIgnored) {
  OrderBook orderBook;

  orderBook.start();

  ErrorParse error;
  error.parse_error = "Test error";
  ParsedUserData parsedEvent = error;

  // Should not crash or cause issues
  orderBook.enqueue(std::move(parsedEvent));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Ensure order book is still functional
  auto event = createOrderTradeUpdate("BTCUSDT", "client-1");
  ParsedUserData validEvent = event;
  orderBook.enqueue(std::move(validEvent));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto order = orderBook.getOrderByClientId("client-1");
  EXPECT_TRUE(order.has_value());
}
