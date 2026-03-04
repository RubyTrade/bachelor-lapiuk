#include <gtest/gtest.h>

#include "core/account_manager/account_controller.hpp"

#include <thread>
#include <vector>

// AccountConfig Tests
TEST(AccountConfig, DefaultLeverageIsOne) {
  AccountConfig cfg;
  EXPECT_EQ(cfg.getLeverageConfig("BTCUSDT"), 1u);
  EXPECT_EQ(cfg.getLeverageConfig("ETHUSDT"), 1u);
  EXPECT_EQ(cfg.getLeverageConfig(""), 1u);
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

  std::vector<std::pair<std::string, uint32_t>> symbols = {{"BTCUSDT", 20},
                                                           {"ETHUSDT", 10},
                                                           {"BNBUSDT", 15},
                                                           {"ADAUSDT", 5},
                                                           {"DOTUSDT", 8}};

  for (const auto &[symbol, leverage] : symbols) {
    cfg.updateLeverage(symbol, leverage);
  }

  for (const auto &[symbol, leverage] : symbols) {
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

  for (auto &thread : threads) {
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

  for (auto &thread : threads) {
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

// New functionality tests

// SymbolConfig tests
TEST(AccountConfig, DefaultSymbolConfigNotFound) {
  AccountConfig cfg;
  auto config = cfg.getSymbolConfig("BTCUSDT");
  EXPECT_FALSE(config.has_value());
}

TEST(AccountConfig, UpdateAndGetSymbolConfig) {
  AccountConfig cfg;

  SymbolConfig btcConfig;
  btcConfig.leverage = 10;
  btcConfig.marginType = MARGIN_TYPE::CROSSED;
  btcConfig.isActive = true;

  cfg.updateSymbolConfig("BTCUSDT", btcConfig);

  auto retrievedConfig = cfg.getSymbolConfig("BTCUSDT");
  ASSERT_TRUE(retrievedConfig.has_value());
  EXPECT_EQ(retrievedConfig->leverage, 10u);
  EXPECT_EQ(retrievedConfig->marginType, MARGIN_TYPE::CROSSED);
  EXPECT_TRUE(retrievedConfig->isActive);
}

TEST(AccountConfig, UpdateSymbolConfigMultipleSymbols) {
  AccountConfig cfg;

  SymbolConfig config1{10, MARGIN_TYPE::CROSSED, true};
  SymbolConfig config2{5, MARGIN_TYPE::ISOLATED, false};

  cfg.updateSymbolConfig("BTCUSDT", config1);
  cfg.updateSymbolConfig("ETHUSDT", config2);

  auto btcConfig = cfg.getSymbolConfig("BTCUSDT");
  auto ethConfig = cfg.getSymbolConfig("ETHUSDT");

  ASSERT_TRUE(btcConfig.has_value());
  ASSERT_TRUE(ethConfig.has_value());
  EXPECT_EQ(btcConfig->leverage, 10u);
  EXPECT_EQ(ethConfig->leverage, 5u);
}

// Margin Type tests
TEST(AccountConfig, DefaultMarginTypeIsCrossed) {
  AccountConfig cfg;
  EXPECT_EQ(cfg.getMarginType("BTCUSDT"), MARGIN_TYPE::CROSSED);
}

TEST(AccountConfig, SetAndGetMarginType) {
  AccountConfig cfg;

  cfg.setMarginType("BTCUSDT", MARGIN_TYPE::ISOLATED);
  EXPECT_EQ(cfg.getMarginType("BTCUSDT"), MARGIN_TYPE::ISOLATED);

  cfg.setMarginType("ETHUSDT", MARGIN_TYPE::CROSSED);
  EXPECT_EQ(cfg.getMarginType("ETHUSDT"), MARGIN_TYPE::CROSSED);
}

TEST(AccountConfig, UpdateMarginTypeMultipleTimes) {
  AccountConfig cfg;

  cfg.setMarginType("BTCUSDT", MARGIN_TYPE::ISOLATED);
  EXPECT_EQ(cfg.getMarginType("BTCUSDT"), MARGIN_TYPE::ISOLATED);

  cfg.setMarginType("BTCUSDT", MARGIN_TYPE::CROSSED);
  EXPECT_EQ(cfg.getMarginType("BTCUSDT"), MARGIN_TYPE::CROSSED);
}

// Permissions tests
TEST(AccountConfig, DefaultPermissions) {
  AccountConfig cfg;
  auto permissions = cfg.getPermissions();

  EXPECT_FALSE(permissions.canTrade);
  EXPECT_FALSE(permissions.canDeposit);
  EXPECT_FALSE(permissions.canWithdraw);
  EXPECT_EQ(permissions.feeTier, 0u);
}

TEST(AccountConfig, UpdatePermissions) {
  AccountConfig cfg;

  AccountPermissions permissions;
  permissions.canTrade = true;
  permissions.canDeposit = true;
  permissions.canWithdraw = false;
  permissions.feeTier = 2;

  cfg.updatePermissions(permissions);

  auto retrieved = cfg.getPermissions();
  EXPECT_TRUE(retrieved.canTrade);
  EXPECT_TRUE(retrieved.canDeposit);
  EXPECT_FALSE(retrieved.canWithdraw);
  EXPECT_EQ(retrieved.feeTier, 2u);
}

TEST(AccountConfig, CanTradeMethod) {
  AccountConfig cfg;

  EXPECT_FALSE(cfg.canTrade());

  AccountPermissions permissions;
  permissions.canTrade = true;
  cfg.updatePermissions(permissions);

  EXPECT_TRUE(cfg.canTrade());
}

// Risk Management tests
TEST(AccountConfig, DefaultRiskConfig) {
  AccountConfig cfg;
  auto riskConfig = cfg.getRiskConfig();

  EXPECT_EQ(riskConfig.maxLeverage, 20u);
  EXPECT_FALSE(riskConfig.autoReduceOnMarginCall);
}

TEST(AccountConfig, UpdateRiskConfig) {
  AccountConfig cfg;

  RiskManagementConfig riskConfig;
  riskConfig.maxLeverage = 10;
  riskConfig.maxPositionSize = Fixed::str_to_fixed("1.0");
  riskConfig.maxTotalPositionValue = Fixed::str_to_fixed("100000.0");
  riskConfig.autoReduceOnMarginCall = true;

  cfg.updateRiskConfig(riskConfig);

  auto retrieved = cfg.getRiskConfig();
  EXPECT_EQ(retrieved.maxLeverage, 10u);
  EXPECT_EQ(retrieved.maxPositionSize.to_string(), "1.0");
  EXPECT_EQ(retrieved.maxTotalPositionValue.to_string(), "100000.0");
  EXPECT_TRUE(retrieved.autoReduceOnMarginCall);
}

// Commission Rate tests
TEST(AccountConfig, DefaultCommissionRates) {
  AccountConfig cfg;
  auto [maker, taker] = cfg.getCommissionRate("BTCUSDT");

  EXPECT_EQ(maker.to_string(), "0.0000");
  EXPECT_EQ(taker.to_string(), "0.0000");
}

TEST(AccountConfig, UpdateCommissionRate) {
  AccountConfig cfg;

  Fixed maker = Fixed::str_to_fixed("0.0002");
  Fixed taker = Fixed::str_to_fixed("0.0004");

  cfg.updateCommissionRate("BTCUSDT", maker, taker);

  auto [retrievedMaker, retrievedTaker] = cfg.getCommissionRate("BTCUSDT");
  EXPECT_EQ(retrievedMaker.to_string(), "0.0002");
  EXPECT_EQ(retrievedTaker.to_string(), "0.0004");
}

TEST(AccountConfig, CommissionRatesMultipleSymbols) {
  AccountConfig cfg;

  cfg.updateCommissionRate("BTCUSDT", Fixed::str_to_fixed("0.0001"),
                           Fixed::str_to_fixed("0.0002"));
  cfg.updateCommissionRate("ETHUSDT", Fixed::str_to_fixed("0.00015"),
                           Fixed::str_to_fixed("0.00025"));

  auto [btcMaker, btcTaker] = cfg.getCommissionRate("BTCUSDT");
  auto [ethMaker, ethTaker] = cfg.getCommissionRate("ETHUSDT");

  EXPECT_EQ(btcMaker.to_string(), "0.0001");
  EXPECT_EQ(btcTaker.to_string(), "0.0002");
  EXPECT_EQ(ethMaker.to_string(), "0.00015");
  EXPECT_EQ(ethTaker.to_string(), "0.00025");
}

// Active Symbols tests
TEST(AccountConfig, DefaultNoActiveSymbols) {
  AccountConfig cfg;
  auto activeSymbols = cfg.getActiveSymbols();
  EXPECT_TRUE(activeSymbols.empty());
}

TEST(AccountConfig, GetActiveSymbols) {
  AccountConfig cfg;

  SymbolConfig config1{10, MARGIN_TYPE::CROSSED, true};
  SymbolConfig config2{5, MARGIN_TYPE::ISOLATED, false};
  SymbolConfig config3{8, MARGIN_TYPE::CROSSED, true};

  cfg.updateSymbolConfig("BTCUSDT", config1);
  cfg.updateSymbolConfig("ETHUSDT", config2);
  cfg.updateSymbolConfig("BNBUSDT", config3);

  auto activeSymbols = cfg.getActiveSymbols();

  EXPECT_EQ(activeSymbols.size(), 2u);
  EXPECT_TRUE(activeSymbols.count("BTCUSDT") > 0);
  EXPECT_TRUE(activeSymbols.count("BNBUSDT") > 0);
  EXPECT_FALSE(activeSymbols.count("ETHUSDT") > 0);
}

TEST(AccountConfig, SetSymbolActive) {
  AccountConfig cfg;

  SymbolConfig config{10, MARGIN_TYPE::CROSSED, true};
  cfg.updateSymbolConfig("BTCUSDT", config);

  auto activeSymbols = cfg.getActiveSymbols();
  EXPECT_EQ(activeSymbols.size(), 1u);

  cfg.setSymbolActive("BTCUSDT", false);
  activeSymbols = cfg.getActiveSymbols();
  EXPECT_EQ(activeSymbols.size(), 0u);

  cfg.setSymbolActive("BTCUSDT", true);
  activeSymbols = cfg.getActiveSymbols();
  EXPECT_EQ(activeSymbols.size(), 1u);
}

// Thread safety tests for new features
TEST(AccountConfig, SymbolConfigThreadSafety) {
  AccountConfig cfg;
  const int numThreads = 10;

  std::vector<std::thread> threads;

  for (int t = 0; t < numThreads; ++t) {
    threads.emplace_back([&cfg, t]() {
      std::string symbol = "SYM" + std::to_string(t);
      SymbolConfig config{static_cast<uint32_t>(t + 1), MARGIN_TYPE::CROSSED,
                          true};
      cfg.updateSymbolConfig(symbol, config);
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  for (int t = 0; t < numThreads; ++t) {
    std::string symbol = "SYM" + std::to_string(t);
    auto config = cfg.getSymbolConfig(symbol);
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->leverage, static_cast<uint32_t>(t + 1));
  }
}

TEST(AccountConfig, CommissionRateThreadSafety) {
  AccountConfig cfg;
  const int numThreads = 10;

  std::vector<std::thread> threads;

  for (int t = 0; t < numThreads; ++t) {
    threads.emplace_back([&cfg, t]() {
      std::string symbol = "SYM" + std::to_string(t);
      for (int i = 0; i < 100; ++i) {
        cfg.updateCommissionRate(symbol, Fixed(i, 4), Fixed(i * 2, 4));
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  // Just verify no crash
  for (int t = 0; t < numThreads; ++t) {
    std::string symbol = "SYM" + std::to_string(t);
    auto [maker, taker] = cfg.getCommissionRate(symbol);
    (void)maker;
    (void)taker;
  }
}
