#include <chrono>
#include <gtest/gtest.h>

#include "core/controllers/user_data_stream_controller.hpp"
#include "core/parsers/user_data_stream_parser.hpp"

#include <atomic>
#include <thread>
#include <vector>

using namespace UserData;

// Mock listener for testing
class MockUserEventListener : public IUserEventListener {
public:
  MockUserEventListener() : eventCount(0) {}

  void enqueue(ParsedUserData event) override {
    events.push_back(event);
    eventCount.fetch_add(1);
  }

  void start() override {}
  void stop() override {}

  std::vector<ParsedUserData> events;
  std::atomic<int> eventCount;
};

// UserEventPublisher Tests
TEST(UserEventPublisher, NoListenersByDefault) {
  UserEventPublisher publisher;

  // Should not crash when publishing without listeners
  ErrorParse error;
  error.parse_error = "test";
  ParsedUserData event = error;

  EXPECT_NO_THROW(publisher.publish(event));
}

TEST(UserEventPublisher, SubscribeSingleListener) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  publisher.subscribe(&listener);

  ErrorParse error;
  error.parse_error = "test event";
  ParsedUserData event = error;

  publisher.publish(event);

  EXPECT_EQ(listener.eventCount.load(), 1);
  ASSERT_EQ(listener.events.size(), 1u);
}

TEST(UserEventPublisher, SubscribeMultipleListeners) {
  UserEventPublisher publisher;
  MockUserEventListener listener1;
  MockUserEventListener listener2;
  MockUserEventListener listener3;

  publisher.subscribe(&listener1);
  publisher.subscribe(&listener2);
  publisher.subscribe(&listener3);

  ErrorParse error;
  error.parse_error = "broadcast";
  ParsedUserData event = error;

  publisher.publish(event);

  EXPECT_EQ(listener1.eventCount.load(), 1);
  EXPECT_EQ(listener2.eventCount.load(), 1);
  EXPECT_EQ(listener3.eventCount.load(), 1);
}

TEST(UserEventPublisher, SubscribeNullptrIsIgnored) {
  UserEventPublisher publisher;

  publisher.subscribe(nullptr);

  ErrorParse error;
  ParsedUserData event = error;

  // Should not crash
  EXPECT_NO_THROW(publisher.publish(event));
}

TEST(UserEventPublisher, SubscribeSameListenerTwice) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  publisher.subscribe(&listener);
  publisher.subscribe(&listener);

  ErrorParse error;
  ParsedUserData event = error;

  publisher.publish(event);

  // Should only receive event once (no duplicates)
  EXPECT_EQ(listener.eventCount.load(), 1);
}

TEST(UserEventPublisher, UnsubscribeListener) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  publisher.subscribe(&listener);
  publisher.unsubscribe(&listener);

  ErrorParse error;
  ParsedUserData event = error;

  publisher.publish(event);

  EXPECT_EQ(listener.eventCount.load(), 0);
}

TEST(UserEventPublisher, UnsubscribeNonExistentListener) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  // Should not crash
  EXPECT_NO_THROW(publisher.unsubscribe(&listener));
}

TEST(UserEventPublisher, UnsubscribeOneOfMultipleListeners) {
  UserEventPublisher publisher;
  MockUserEventListener listener1;
  MockUserEventListener listener2;
  MockUserEventListener listener3;

  publisher.subscribe(&listener1);
  publisher.subscribe(&listener2);
  publisher.subscribe(&listener3);

  publisher.unsubscribe(&listener2);

  ErrorParse error;
  ParsedUserData event = error;

  publisher.publish(event);

  EXPECT_EQ(listener1.eventCount.load(), 1);
  EXPECT_EQ(listener2.eventCount.load(), 0);
  EXPECT_EQ(listener3.eventCount.load(), 1);
}

TEST(UserEventPublisher, PublishMultipleEvents) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  publisher.subscribe(&listener);

  for (int i = 0; i < 10; ++i) {
    ErrorParse error;
    error.parse_error = "event " + std::to_string(i);
    ParsedUserData event = error;
    publisher.publish(event);
  }

  EXPECT_EQ(listener.eventCount.load(), 10);
  EXPECT_EQ(listener.events.size(), 10u);
}

TEST(UserEventPublisher, DifferentEventTypes) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  publisher.subscribe(&listener);

  // ErrorParse event
  ErrorParse error;
  error.parse_error = "error";
  publisher.publish(ParsedUserData(error));

  // TradeLiteEvent
  TradeLiteEvent trade;
  trade.symbol = "BTCUSDT";
  trade.orderId = 123;
  publisher.publish(ParsedUserData(trade));

  // OrderTradeUpdateEvent
  OrderTradeUpdateEvent orderUpdate;
  orderUpdate.symbol = "ETHUSDT";
  orderUpdate.orderId = 456;
  publisher.publish(ParsedUserData(orderUpdate));

  EXPECT_EQ(listener.eventCount.load(), 3);
}

TEST(UserEventPublisher, ResubscribeAfterUnsubscribe) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  publisher.subscribe(&listener);
  publisher.unsubscribe(&listener);
  publisher.subscribe(&listener);

  ErrorParse error;
  ParsedUserData event = error;

  publisher.publish(event);

  EXPECT_EQ(listener.eventCount.load(), 1);
}

TEST(UserEventPublisher, ThreadSafePublish) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  publisher.subscribe(&listener);

  const int numThreads = 10;
  const int eventsPerThread = 100;
  std::vector<std::thread> threads;

  listener.events.reserve(numThreads * eventsPerThread * 2);

  for (int t = 0; t < numThreads; ++t) {
    threads.emplace_back([&publisher, eventsPerThread]() {
      for (int i = 0; i < eventsPerThread; ++i) {
        ErrorParse error;
        error.parse_error = "concurrent event";
        ParsedUserData event = error;
        publisher.publish(event);
      }
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_EQ(listener.eventCount.load(), numThreads * eventsPerThread);
}

TEST(UserEventPublisher, ThreadSafeSubscribeUnsubscribe) {
  UserEventPublisher publisher;
  std::vector<std::unique_ptr<MockUserEventListener>> listeners;

  const int numListeners = 50;
  for (int i = 0; i < numListeners; ++i) {
    listeners.push_back(std::make_unique<MockUserEventListener>());
  }

  std::vector<std::thread> threads;

  // Subscribe threads
  for (int i = 0; i < numListeners / 2; ++i) {
    threads.emplace_back([&publisher, &listeners, i]() {
      publisher.subscribe(listeners[i].get());
    });
  }

  // Unsubscribe threads
  for (int i = numListeners / 2; i < numListeners; ++i) {
    threads.emplace_back([&publisher, &listeners, i]() {
      publisher.subscribe(listeners[i].get());
      publisher.unsubscribe(listeners[i].get());
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  // Publish an event
  ErrorParse error;
  ParsedUserData event = error;
  publisher.publish(event);

  // First half should receive event
  for (int i = 0; i < numListeners / 2; ++i) {
    EXPECT_GE(listeners[i]->eventCount.load(), 0);
  }
}

TEST(UserEventPublisher, MultiplePublishersIndependent) {
  UserEventPublisher publisher1;
  UserEventPublisher publisher2;

  MockUserEventListener listener1;
  MockUserEventListener listener2;

  publisher1.subscribe(&listener1);
  publisher2.subscribe(&listener2);

  ErrorParse error;
  ParsedUserData event = error;

  publisher1.publish(event);

  EXPECT_EQ(listener1.eventCount.load(), 1);
  EXPECT_EQ(listener2.eventCount.load(), 0);

  publisher2.publish(event);

  EXPECT_EQ(listener1.eventCount.load(), 1);
  EXPECT_EQ(listener2.eventCount.load(), 1);
}

TEST(UserEventPublisher, ListenerReceivesCorrectEventData) {
  UserEventPublisher publisher;
  MockUserEventListener listener;

  publisher.subscribe(&listener);

  TradeLiteEvent trade;
  trade.symbol = "BTCUSDT";
  trade.orderId = 12345;
  trade.price = Fixed("50000.0");

  ParsedUserData event = trade;
  publisher.publish(event);

  ASSERT_EQ(listener.events.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<TradeLiteEvent>(listener.events[0]));

  const auto &receivedTrade = std::get<TradeLiteEvent>(listener.events[0]);
  EXPECT_EQ(receivedTrade.symbol, "BTCUSDT");
  EXPECT_EQ(receivedTrade.orderId, 12345u);
}

TEST(UserEventPublisher, UnsubscribeAllListeners) {
  UserEventPublisher publisher;
  std::vector<std::unique_ptr<MockUserEventListener>> listeners;

  const int count = 10;
  for (int i = 0; i < count; ++i) {
    listeners.push_back(std::make_unique<MockUserEventListener>());
    publisher.subscribe(listeners[i].get());
  }

  for (int i = 0; i < count; ++i) {
    publisher.unsubscribe(listeners[i].get());
  }

  ErrorParse error;
  ParsedUserData event = error;
  publisher.publish(event);

  for (int i = 0; i < count; ++i) {
    EXPECT_EQ(listeners[i]->eventCount.load(), 0);
  }
}

TEST(UserEventPublisher, PublishDuringSubscription) {
  UserEventPublisher publisher;
  MockUserEventListener listener1;
  MockUserEventListener listener2;

  publisher.subscribe(&listener1);

  std::thread publishThread([&publisher]() {
    for (int i = 0; i < 100; ++i) {
      ErrorParse error;
      ParsedUserData event = error;
      publisher.publish(event);
    }
  });

  std::thread subscribeThread([&publisher, &listener2]() {
    for (int i = 0; i < 50; ++i) {
      publisher.subscribe(&listener2);
      publisher.unsubscribe(&listener2);
    }
  });

  publishThread.join();
  subscribeThread.join();

  // Should not crash
  EXPECT_TRUE(true);
}
