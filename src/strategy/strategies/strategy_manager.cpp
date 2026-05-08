#include "strategy_manager.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/helper_utils.hpp"
#include "strategy/strategies/data_dispatcher.hpp"
#include "strategy/strategies/mean_reversion.hpp"
#include <memory>
#include <string>
#include <utility>

StrategyManager::StrategyManager(Trading::TradingStreamController *controller,
                                 OrderBook *order_book,
                                 AccountController *account_controller)
    : m_dispatcher(std::make_unique<MarketDataDispatcher>()),
      m_trading_controller(controller), m_order_book(order_book),
      m_account_controller(account_controller) {
  m_order_book->register_update_callback(
      [this]() { this->on_order_book_update(); });
}

StrategyInfo StrategyManager::create_strategy(STRATEGIES strategy) {
  std::lock_guard<std::mutex> lock(m_mtx);

  if (strategy == STRATEGIES::UNKNOWN)
    return {};

  size_t strats_count = m_strategies_info.size();
  std::string unique_strat_name = type_to_str(STRATEGIES_STR, strategy) + "_" +
                                  std::to_string(strats_count);

  StrategyInfo strat_info{strategy, unique_strat_name};

  switch (strategy) {
  case STRATEGIES::MEAN_REVERSION: {
    std::unique_ptr<IStrategy> mean =
        std::make_unique<MeanReversion>(unique_strat_name);
    m_dispatcher->subscribe_to_dispatcher(mean.get());

    m_strategies[strategy][unique_strat_name] = std::move(mean);
    break;
  }
  case STRATEGIES::UNKNOWN:
    return {};
  }

  m_strategies_info.push_back(strat_info);

  return strat_info;
}

bool StrategyManager::deactivate_strategy(const StrategyInfo &strategy_info) {
  std::lock_guard<std::mutex> lock(m_mtx);

  auto it = m_strategies.find(strategy_info.strategy);
  if (it != m_strategies.end()) {
    return m_strategies[strategy_info.strategy].erase(
        strategy_info.strategy_str);
  }
  return false;
}
