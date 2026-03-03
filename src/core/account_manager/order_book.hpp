#ifndef ORDER_BOOK_HPP
#define ORDER_BOOK_HPP

#include "core/controllers/user_data_stream_controller.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

struct OrderEntry {
  // --- Identity ---
  std::string symbol{};
  uint64_t orderId = 0;
  std::string clientOrderId{};

  // --- Type ---
  ORDER_SIDE side{};            // BUY / SELL
  ORDER_TYPE type{};            // MARKET / LIMIT
  POSITION_SIDE positionSide{}; // BOTH / LONG / SHORT

  // --- Status ---
  TIME_IN_FORCE tif{};            // NEW / TRADE / CANCELED
  EXECUTION_TYPE executionType{}; // NEW / TRADE / CANCELED
  ORDER_STATUS orderStatus{};     // NEW / FILLED / PARTIALLY_FILLED

  // --- Quantity & Price ---
  Fixed origQty{};
  Fixed executedQty{};
  Fixed lastQty{};
  Fixed lastPrice{};
  Fixed price{};
  Fixed avgPrice{};
  bool reduceOnly = false;

  // --- Fees ---
  Fixed commission{};
  std::string commissionAsset{};

  // --- Time ---
  uint64_t eventTime = 0;
  uint64_t tradeTime = 0;

  // --- PnL ---
  Fixed realizedPnL{};

  // To persist data, so trade lite won't override the full data
  USER_DATA_EVENT_TYPE fulfilledBy{};
};

class AccountOrders {
public:
  std::optional<OrderEntry>
  getOrderByClientId(const std::string &clientOrderId);

  void tryRemove(const std::string &clientOrderId);
  void tryEmplace(const std::string &clientOrderId,
                  std::function<void(OrderEntry &)> fn);

private:
  // key - clientOrderId
  std::unordered_map<std::string, OrderEntry> m_orders;
  std::mutex m_ordersMutex;
};

class OrderBook : public UserData::IUserEventListener {
public:
  OrderBook();
  ~OrderBook();

  void enqueue(UserData::ParsedUserData event) override;
  void stop() override;
  void start() override;

  std::optional<OrderEntry>
  getOrderByClientId(const std::string &clientOrderId);

  uint64_t getLastUpdateTime() const { return m_lastUpdateTime; }

private:
  void _runProcessingThread();
  void _listenToUpdates();

  void updateOrCreateOrder(const UserData::ParsedUserData &event);

  void _updateLastUpdateTime();

  void _updateOrCreateTradeLite(const UserData::TradeLiteEvent &event);
  void
  _updateOrCreateOrderTradeUpdate(const UserData::OrderTradeUpdateEvent &event);

private:
  std::unique_ptr<AccountOrders> m_orders;

  std::unique_ptr<Queue<UserData::ParsedUserData>> m_eventQueue;
  std::atomic_bool m_isQueueActive{false};

  std::unique_ptr<Thread> m_processingThread;

  uint64_t m_lastUpdateTime;
};

#endif // ORDER_BOOK_HPP
