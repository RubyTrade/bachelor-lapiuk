#include <gtest/gtest.h>

#include "core/account_manager/account_controller.hpp"

#include <thread>
#include <vector>

// AccountConfig Tests
TEST(AccountConfig, DefaultLeverageIsZero) {
    AccountConfig cfg;
    EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 0u);
    EXPECT_EQ(cfg.getLeverageConfig("ETHUSDT"), 0u);
    EXPECT_EQ(cfg.getLeverageConfig(""), 0u);
}

TEST(AccountConfig, DefaultMultiAssetModeIsFalse) {
    AccountConfig cfg;
    EXPECT_FALSE(cfg.getMultiAssetMode());
}

TEST(AccountConfig, UpdateAndGetLeverage) {
    AccountConfig cfg;
    
    cfg.updateLeverage("BTCUSDT", 20);
    EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 20u);
    
    cfg.updateLeverage("ETHUSDT", 10);
    EXPECT_EQ(cfg.getLeverageConfig("ETHUSDT"), 10u);
    EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 20u);
}

TEST(AccountConfig, UpdateLeverageMultipleTimes) {
    AccountConfig cfg;
    
    cfg.updateLeverage("BTCUSDT", 5);
    EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 5u);
    
    cfg.updateLeverage("BTCUSDT", 10);
    EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 10u);
    
    cfg.updateLeverage("BTCUSDT", 20);
    EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 20u);
}

TEST(AccountConfig, UpdateMultiAssetMode) {
    AccountConfig cfg;
    
    cfg.updateMultiAssetMode(true);
    EXPECT_TRUE(cfg.getMultiAssetMode());
    
    cfg.updateMultiAssetMode(false);
    EXPECT_FALSE(cfg.getMultiAssetMode());
    
    cfg.updateMultiAssetMode(true);
    EXPECT_TRUE(cfg.getMultiAssetMode());
}

TEST(AccountConfig, MultipleLeverageSymbols) {
    AccountConfig cfg;
    
    std::vector<std::pair<std::string, uint32_t>> symbols = {
        {"BTCUSDT", 20},
        {"ETHUSDT", 10},
        {"BNBUSDT", 15},
        {"ADAUSDT", 5},
        {"DOTUSDT", 8}
    };
    
    for (const auto& [symbol, leverage] : symbols) {
        cfg.updateLeverage(symbol, leverage);
    }
    
    for (const auto& [symbol, leverage] : symbols) {
        EXPECT_EQ(cfg.getLeverageConfig(symbol), leverage);
    }
}

TEST(AccountConfig, LeverageThreadSafety) {
    AccountConfig cfg;
    const int numThreads = 10;
    const int numUpdates = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&cfg, t]() {
            std::string symbol = "SYM" + std::to_string(t);
            for (int i = 0; i < numUpdates; ++i) {
                cfg.updateLeverage(symbol, i + 1);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    for (int t = 0; t < numThreads; ++t) {
        std::string symbol = "SYM" + std::to_string(t);
        EXPECT_EQ(cfg.getLeverageConfig(symbol), numUpdates);
    }
}

TEST(AccountConfig, MultiAssetModeThreadSafety) {
    AccountConfig cfg;
    const int numThreads = 10;
    const int numUpdates = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&cfg, t]() {
            for (int i = 0; i < numUpdates; ++i) {
                cfg.updateMultiAssetMode(t % 2 == 0);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Just ensure no crash, final value is non-deterministic
    bool finalValue = cfg.getMultiAssetMode();
    (void)finalValue; // suppress unused warning
}

TEST(AccountConfig, LeverageZeroIsValid) {
    AccountConfig cfg;
    cfg.updateLeverage("BTCUSDT", 0);
    EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 0u);
}

TEST(AccountConfig, LeverageMaxValue) {
    AccountConfig cfg;
    cfg.updateLeverage("BTCUSDT", 125);
    EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 125u);
}
