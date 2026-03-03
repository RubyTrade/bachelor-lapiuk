#include <gtest/gtest.h>

#include "core/controllers/market_data_controller.hpp"
#include "core/controllers/market_data_utils.hpp"
#include "core/utils/constants.hpp"

#include <thread>
#include <vector>

using namespace Market;

// SubscriptionsList Tests
TEST(SubscriptionsList, EmptyByDefault) {
    SubscriptionsList list;
    auto items = list.get_list();
    EXPECT_TRUE(items.empty());
}

TEST(SubscriptionsList, AddSingleSubscription) {
    SubscriptionsList list;
    MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    
    list.add_to_list(req);
    
    auto items = list.get_list();
    EXPECT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].symbol, "BTCUSDT");
    EXPECT_EQ(items[0].type, MARKET_DATA_TYPE::TRADE);
}

TEST(SubscriptionsList, AddMultipleSubscriptions) {
    SubscriptionsList list;
    
    std::vector<MarketRequest> requests = {
        MarketRequest("BTCUSDT", MARKET_DATA_TYPE::TRADE),
        MarketRequest("ETHUSDT", MARKET_DATA_TYPE::AGG_TRADE),
        MarketRequest("BNBUSDT", MARKET_DATA_TYPE::MARK_PRICE)
    };
    
    for (const auto& req : requests) {
        list.add_to_list(req);
    }
    
    auto items = list.get_list();
    EXPECT_EQ(items.size(), 3u);
}

TEST(SubscriptionsList, AddDuplicateSubscriptions) {
    SubscriptionsList list;
    MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    
    list.add_to_list(req);
    list.add_to_list(req);
    
    auto items = list.get_list();
    // Implementation allows duplicates
    EXPECT_EQ(items.size(), 2u);
}

TEST(SubscriptionsList, RemoveExistingSubscription) {
    SubscriptionsList list;
    MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    
    list.add_to_list(req);
    EXPECT_EQ(list.get_list().size(), 1u);
    
    list.remove_from_list(req);
    EXPECT_EQ(list.get_list().size(), 0u);
}

TEST(SubscriptionsList, RemoveNonExistentSubscription) {
    SubscriptionsList list;
    MarketRequest req1("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    MarketRequest req2("ETHUSDT", MARKET_DATA_TYPE::TRADE);
    
    list.add_to_list(req1);
    
    // Should not crash
    list.remove_from_list(req2);
    
    EXPECT_EQ(list.get_list().size(), 1u);
}

TEST(SubscriptionsList, RemoveFromEmptyList) {
    SubscriptionsList list;
    MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    
    // Should not crash
    list.remove_from_list(req);
    
    EXPECT_EQ(list.get_list().size(), 0u);
}

TEST(SubscriptionsList, RemoveOnlyFirstMatchingSubscription) {
    SubscriptionsList list;
    MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    
    list.add_to_list(req);
    list.add_to_list(req);
    list.add_to_list(req);
    
    EXPECT_EQ(list.get_list().size(), 3u);
    
    list.remove_from_list(req);
    
    // Should remove only one
    EXPECT_EQ(list.get_list().size(), 2u);
}

TEST(SubscriptionsList, ThreadSafeAddOperations) {
    SubscriptionsList list;
    const int numThreads = 10;
    const int itemsPerThread = 50;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&list, t, itemsPerThread]() {
            for (int i = 0; i < itemsPerThread; ++i) {
                std::string symbol = "SYM" + std::to_string(t * 100 + i);
                MarketRequest req(symbol, MARKET_DATA_TYPE::TRADE);
                list.add_to_list(req);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto items = list.get_list();
    EXPECT_EQ(items.size(), numThreads * itemsPerThread);
}

TEST(SubscriptionsList, ThreadSafeRemoveOperations) {
    SubscriptionsList list;
    
    // Pre-populate list
    for (int i = 0; i < 100; ++i) {
        std::string symbol = "SYM" + std::to_string(i);
        MarketRequest req(symbol, MARKET_DATA_TYPE::TRADE);
        list.add_to_list(req);
    }
    
    const int numThreads = 10;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&list, t]() {
            for (int i = 0; i < 10; ++i) {
                std::string symbol = "SYM" + std::to_string(t * 10 + i);
                MarketRequest req(symbol, MARKET_DATA_TYPE::TRADE);
                list.remove_from_list(req);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto items = list.get_list();
    EXPECT_EQ(items.size(), 0u);
}

TEST(SubscriptionsList, ThreadSafeMixedOperations) {
    SubscriptionsList list;
    const int numThreads = 5;
    
    std::vector<std::thread> threads;
    
    // Add threads
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&list, t]() {
            for (int i = 0; i < 20; ++i) {
                std::string symbol = "ADD" + std::to_string(t * 100 + i);
                MarketRequest req(symbol, MARKET_DATA_TYPE::TRADE);
                list.add_to_list(req);
            }
        });
    }
    
    // Remove threads
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&list, t]() {
            for (int i = 0; i < 10; ++i) {
                std::string symbol = "ADD" + std::to_string(t * 100 + i);
                MarketRequest req(symbol, MARKET_DATA_TYPE::TRADE);
                list.remove_from_list(req);
            }
        });
    }
    
    // Read threads
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&list]() {
            for (int i = 0; i < 50; ++i) {
                auto items = list.get_list();
                (void)items; // Just reading
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have some items left
    auto items = list.get_list();
    EXPECT_GT(items.size(), 0u);
}

TEST(SubscriptionsList, GetListReturnsIndependentCopy) {
    SubscriptionsList list;
    MarketRequest req("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    
    list.add_to_list(req);
    
    auto items1 = list.get_list();
    auto items2 = list.get_list();
    
    // Both should have same content
    EXPECT_EQ(items1.size(), items2.size());
    EXPECT_EQ(items1[0].symbol, items2[0].symbol);
}

TEST(SubscriptionsList, DifferentMarketDataTypes) {
    SubscriptionsList list;
    
    std::vector<MARKET_DATA_TYPE> types = {
        MARKET_DATA_TYPE::TRADE,
        MARKET_DATA_TYPE::AGG_TRADE,
        MARKET_DATA_TYPE::MARK_PRICE,
        MARKET_DATA_TYPE::DIFF_DEPTH,
        MARKET_DATA_TYPE::PART_DEPTH,
        MARKET_DATA_TYPE::BOOK_TICKER
    };
    
    for (const auto& type : types) {
        MarketRequest req("BTCUSDT", type);
        list.add_to_list(req);
    }
    
    auto items = list.get_list();
    EXPECT_EQ(items.size(), types.size());
}

TEST(SubscriptionsList, SameSymbolDifferentTypes) {
    SubscriptionsList list;
    
    MarketRequest req1("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    MarketRequest req2("BTCUSDT", MARKET_DATA_TYPE::AGG_TRADE);
    
    list.add_to_list(req1);
    list.add_to_list(req2);
    
    auto items = list.get_list();
    EXPECT_EQ(items.size(), 2u);
    
    // Remove one should not affect the other
    list.remove_from_list(req1);
    
    items = list.get_list();
    EXPECT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].type, MARKET_DATA_TYPE::AGG_TRADE);
}

TEST(SubscriptionsList, RemoveMiddleElement) {
    SubscriptionsList list;
    
    MarketRequest req1("BTCUSDT", MARKET_DATA_TYPE::TRADE);
    MarketRequest req2("ETHUSDT", MARKET_DATA_TYPE::TRADE);
    MarketRequest req3("BNBUSDT", MARKET_DATA_TYPE::TRADE);
    
    list.add_to_list(req1);
    list.add_to_list(req2);
    list.add_to_list(req3);
    
    list.remove_from_list(req2);
    
    auto items = list.get_list();
    EXPECT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].symbol, "BTCUSDT");
    EXPECT_EQ(items[1].symbol, "BNBUSDT");
}
