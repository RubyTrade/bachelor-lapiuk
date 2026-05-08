#ifndef STRATEGY_HPP
#define STRATEGY_HPP

#include "core/controllers/trading_stream_utils.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct OrdersBundle {
  std::string clientOrderId;        // Main order clientOrderId
  Trading::TradeRequest main_order; // Main order LONG / SHORT
  ORDER_SIDE_TYPE side;             // LONG / SHORT
  Fixed price{};                    // Main orders price
  std::vector<Trading::TradeRequest> additional_orders{}; // TP / SL
  ORDER_STATUS order_status = ORDER_STATUS::NEW;

  OrdersBundle(const std::string &order_id, const Trading::TradeRequest &order,
               ORDER_SIDE_TYPE s)
      : clientOrderId(order_id), main_order(order), side(s) {}
};

class OrdersPool {
public:
  void add_order(OrdersBundle order_bundle) {
    std::lock_guard<std::mutex> lock(m_mtx);

    if (m_orders.find(order_bundle.clientOrderId) == m_orders.end()) {
      m_orders.emplace(order_bundle.clientOrderId, order_bundle);
      m_lastOrder = order_bundle.clientOrderId;
    }
  }

  void remove_order(const std::string &clientOrderId) {
    std::lock_guard<std::mutex> lock(m_mtx);

    m_orders.erase(clientOrderId);
  }

  bool is_exist(const std::string &clientOrderId) {
    std::lock_guard<std::mutex> lock(m_mtx);

    auto it = m_orders.find(clientOrderId);
    return it != m_orders.end();
  }

  bool is_flat() {
    std::lock_guard<std::mutex> lock(m_mtx);

    return m_orders.empty();
  }

  bool update_order(OrdersBundle order_bundle) {
    std::lock_guard<std::mutex> lock(m_mtx);

    auto it = m_orders.find(order_bundle.clientOrderId);
    if (it == m_orders.end())
      return false;

    it->second = std::move(order_bundle);
    return true;
  }

  const std::unordered_map<std::string, OrdersBundle> &get_orders() const {
    return m_orders;
  }

  std::string get_last_order_str() const { return m_lastOrder; }

  std::optional<OrdersBundle> get_last_order() {
    std::lock_guard<std::mutex> lock(m_mtx);

    auto it = m_orders.find(m_lastOrder);
    if (it != m_orders.end()) {
      return it->second;
    }
    return std::nullopt;
  }

private:
  std::mutex m_mtx{};
  // key - clientOrderId
  std::unordered_map<std::string, OrdersBundle> m_orders{};
  std::string m_lastOrder{};
};

class IStrategy {
public:
  IStrategy(const std::string &strategy_name) : STRATEGY_STR(strategy_name) {}

  virtual ~IStrategy() = default;
  virtual SIGNAL produce_signal() = 0;
  virtual void on_market_data(const Market::ParsedMarketData &data) = 0;

  std::string getStrategyStr() const { return STRATEGY_STR; }

public:
  OrdersPool order_pool;
  std::string STRATEGY_STR;
};

#endif // STRATEGY_HPP
