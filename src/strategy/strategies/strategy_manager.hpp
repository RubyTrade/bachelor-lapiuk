#ifndef STRATEGY_MANAGER_HPP
#define STRATEGY_MANAGER_HPP

#include "core/account_manager/account_controller.hpp"
#include "core/account_manager/order_book.hpp"
#include "core/controllers/trading_stream_controller.hpp"
#include "core/controllers/trading_stream_utils.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/log.hpp"
#include "strategy/strategies/data_dispatcher.hpp"
#include "strategy/strategies/strategy.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct StrategyInfo {
  STRATEGIES strategy;
  std::string strategy_str;
};

class StrategyManager {
public:
  StrategyManager(Trading::TradingStreamController *controller,
                  OrderBook *order_book, AccountController *account_controller);

  StrategyInfo create_strategy(STRATEGIES strategy);
  bool deactivate_strategy(const StrategyInfo &strategy_info);

  void run_loop(const StrategyInfo &strategy_info) {

    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      SIGNAL signal =
          m_strategies[strategy_info.strategy][strategy_info.strategy_str]
              ->produce_signal();
      if (signal != SIGNAL::HOLD) {
        Log::log_info("Signal of " + strategy_info.strategy_str + " | " +
                      type_to_str(SIGNAL_STR, signal));
        process_signal(strategy_info, signal);
      }
    }
  }

  void process_signal(const StrategyInfo &strategy_info, SIGNAL signal) {
    if (signal == SIGNAL::HOLD)
      return;

    if (m_trading_controller) {
      if (signal == SIGNAL::LONG_ENTRY || signal == SIGNAL::SHORT_ENTRY) {
        Trading::TradeRequest req = generateOrder(signal);
        if (!req.isValid())
          return;

        ORDER_SIDE_TYPE side =
            (signal == SIGNAL::LONG_ENTRY ? ORDER_SIDE_TYPE::LONG
                                          : ORDER_SIDE_TYPE::SHORT);
        OrdersBundle bundle{req.getClientOrderId(), req, side};
        m_strategies[strategy_info.strategy][strategy_info.strategy_str]
            ->order_pool.add_order(bundle);
      }
      // otherwise it's a close signal
      else {
        ORDER_SIDE_TYPE side =
            (signal == SIGNAL::CLOSE_LONG ? ORDER_SIDE_TYPE::LONG
                                          : ORDER_SIDE_TYPE::SHORT);
        const auto &orders =
            m_strategies[strategy_info.strategy][strategy_info.strategy_str]
                ->order_pool.get_orders();
        for (auto &order : orders) {
          if (order.second.side == side) {
            if (!order.second.additional_orders.empty()) {
              for (auto &add_order : order.second.additional_orders) {
                m_trading_controller->cancel_algo_order(add_order);
              }
            }
            closeOrder(order.second.main_order, signal);
            m_strategies[strategy_info.strategy][strategy_info.strategy_str]
                ->order_pool.remove_order(order.first);
            return;
          }
        }
      }
    }
  }

  void on_order_book_update() {
    for (auto &info : m_strategies_info) {
      OrdersPool &order_pool =
          m_strategies[info.strategy][info.strategy_str]->order_pool;
      const auto &orders = order_pool.get_orders();
      for (auto &order : orders) {
        std::optional<OrderEntry> entry =
            m_order_book->getOrderByClientId(order.first);
        if (entry.has_value()) {
          OrdersBundle bundle = order.second;
          // MARKET fills often have `p` (price) = 0; use avg / last trade
          // price.
          bundle.price = referenceFillPrice(entry.value());
          ORDER_STATUS old_status = bundle.order_status;
          bundle.order_status = entry->orderStatus;
          if (((old_status == ORDER_STATUS::NEW ||
                old_status == ORDER_STATUS::PARTIALLY_FILLED) &&
               entry->orderStatus == ORDER_STATUS::FILLED) &&
              bundle.additional_orders.empty()) {
            if (bundle.price > Fixed{0}) {
              Trading::TradeRequest tp = buildTakeProfitRequest(
                  bundle.main_order, bundle.price, bundle.side);
              if (tp.isValid() &&
                  !m_trading_controller->create_algo_order(tp).empty())
                bundle.additional_orders.emplace_back(std::move(tp));
              Trading::TradeRequest sl = buildStopLossRequest(
                  bundle.main_order, bundle.price, bundle.side);
              if (sl.isValid() &&
                  !m_trading_controller->create_algo_order(sl).empty())
                bundle.additional_orders.emplace_back(std::move(sl));
            } else {
              Log::log_err(
                  "TP/SL skipped: no fill reference price for clientOrderId " +
                  order.first);
            }
          }

          order_pool.update_order(bundle);
          return;
        }
      }
    }
  }

  Trading::TradeRequest generateOrder(SIGNAL signal) {
    Trading::TradeRequest req{};
    if (signal == SIGNAL::LONG_ENTRY) {
      req = Trading::TradeRequest(ORDER_SIDE::BUY, POSITION_SIDE::BOTH,
                                  ORDER_TYPE::MARKET, "SOLUSDT",
                                  Fixed::str_to_fixed("4.5"));

      m_trading_controller->create_order(req);
    } else if (signal == SIGNAL::SHORT_ENTRY) {
      req = Trading::TradeRequest(ORDER_SIDE::SELL, POSITION_SIDE::BOTH,
                                  ORDER_TYPE::MARKET, "SOLUSDT",
                                  Fixed::str_to_fixed("4.5"));

      m_trading_controller->create_order(req);
    }
    return req;
  }

  Trading::TradeRequest closeOrder(const Trading::TradeRequest &order,
                                   SIGNAL signal) {
    Trading::TradeRequest req{};
    if (signal == SIGNAL::CLOSE_LONG) {
      req =
          Trading::TradeRequest(ORDER_SIDE::SELL, POSITION_SIDE::BOTH,
                                ORDER_TYPE::MARKET, "SOLUSDT", order.quantity);
      req.setReduceOnly(true);

      m_trading_controller->create_order(req);
    } else if (signal == SIGNAL::CLOSE_SHORT) {
      req =
          Trading::TradeRequest(ORDER_SIDE::BUY, POSITION_SIDE::BOTH,
                                ORDER_TYPE::MARKET, "SOLUSDT", order.quantity);
      req.setReduceOnly(true);

      m_trading_controller->create_order(req);
    }
    return req;
  }

  static Fixed referenceFillPrice(const OrderEntry &entry) {
    if (entry.price > Fixed{0})
      return entry.price;
    if (entry.avgPrice > Fixed{0})
      return entry.avgPrice;
    if (entry.lastPrice > Fixed{0})
      return entry.lastPrice;
    return Fixed{0};
  }

  static Trading::TradeRequest
  buildTakeProfitRequest(const Trading::TradeRequest &main_order,
                         const Fixed &price, ORDER_SIDE_TYPE side) {
    Trading::TradeRequest req{};
    if (side == ORDER_SIDE_TYPE::LONG) {
      req = Trading::TradeRequest(ORDER_SIDE::SELL, POSITION_SIDE::BOTH,
                                  ORDER_TYPE::TAKE_PROFIT_MARKET, "SOLUSDT");
      req.setStopPrice(price * (Fixed{1} + TP));
      req.setClosePosition(true);
    } else if (side == ORDER_SIDE_TYPE::SHORT) {
      req = Trading::TradeRequest(ORDER_SIDE::BUY, POSITION_SIDE::BOTH,
                                  ORDER_TYPE::TAKE_PROFIT_MARKET, "SOLUSDT");
      req.setStopPrice(price * (Fixed{1} - TP));
      req.setClosePosition(true);
    }
    return req;
  }

  static Trading::TradeRequest
  buildStopLossRequest(const Trading::TradeRequest &main_order,
                       const Fixed &price, ORDER_SIDE_TYPE side) {
    Trading::TradeRequest req{};
    if (side == ORDER_SIDE_TYPE::LONG) {
      req = Trading::TradeRequest(ORDER_SIDE::SELL, POSITION_SIDE::BOTH,
                                  ORDER_TYPE::STOP_MARKET, "SOLUSDT");
      req.setStopPrice(price * (Fixed{1} - SL));
      req.setClosePosition(true);
    } else if (side == ORDER_SIDE_TYPE::SHORT) {
      req = Trading::TradeRequest(ORDER_SIDE::BUY, POSITION_SIDE::BOTH,
                                  ORDER_TYPE::STOP_MARKET, "SOLUSDT");
      req.setStopPrice(price * (Fixed{1} + SL));
      req.setClosePosition(true);
    }
    return req;
  }

  MarketDataDispatcher *getMarketDispatcher() const {
    return m_dispatcher.get();
  }

private:
  std::mutex m_mtx{};

  std::unordered_map<
      STRATEGIES, std::unordered_map<std::string, std::unique_ptr<IStrategy>>>
      m_strategies{};
  std::vector<StrategyInfo> m_strategies_info{};

  std::unique_ptr<MarketDataDispatcher> m_dispatcher;
  Trading::TradingStreamController *m_trading_controller;
  OrderBook *m_order_book;
  AccountController *m_account_controller{};

  // temporary store them here
  static inline const Fixed TP{0.003, 3};
  static inline const Fixed SL{0.002, 3};
};

#endif // STRATEGY_MANAGER_HPP
