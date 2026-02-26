#include "core/account_manager/order_book.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/constants.hpp"

#include <chrono>
#include <mutex>
#include <optional>
#include <variant>

OrderBook::OrderBook() : m_orders(std::make_unique<AccountOrders>()) {
  _updateLastUpdateTime();
}

std::optional<OrderEntry>
AccountOrders::getOrderByClientId(const std::string &clientOrderId) {
  std::lock_guard<std::mutex> lock(m_ordersMutex);
  auto it = m_orders.find(clientOrderId);
  if (it != m_orders.end())
    return it->second;
  return std::nullopt;
}

void AccountOrders::tryRemove(const std::string &clientOrderId) {
  std::lock_guard<std::mutex> lock(m_ordersMutex);

  m_orders.erase(clientOrderId);
}

void AccountOrders::tryEmplace(const std::string &clientOrderId,
                               std::function<void(OrderEntry &)> fn) {
  std::lock_guard<std::mutex> lock(m_ordersMutex);

  auto [it, inserted] = m_orders.try_emplace(clientOrderId);
  fn(it->second);
}

void OrderBook::onEvent(const UserData::ParsedUserData &event) {
  if (!std::holds_alternative<ErrorParse>(event))
    updateOrCreateOrder(event);

  return;
}

std::optional<OrderEntry>
OrderBook::getOrderByClientId(const std::string &clientOrderId) {
  return m_orders->getOrderByClientId(clientOrderId);
}

void OrderBook::updateOrCreateOrder(const UserData::ParsedUserData &event) {
  if (std::holds_alternative<UserData::TradeLiteEvent>(event)) {
    _updateOrCreateTradeLite(std::get<UserData::TradeLiteEvent>(event));

  } else if (std::holds_alternative<UserData::OrderTradeUpdateEvent>(event)) {
    _updateOrCreateOrderTradeUpdate(
        std::get<UserData::OrderTradeUpdateEvent>(event));
  }

  // If we hit else,
  // that means, no update was done
  else {
    return;
  }

  _updateLastUpdateTime();
}

void OrderBook::_updateLastUpdateTime() {
  m_lastUpdateTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
}

void OrderBook::_updateOrCreateTradeLite(
    const UserData::TradeLiteEvent &event) {
  m_orders->tryEmplace(event.clientOrderId, [&event](OrderEntry &entry) {
    if (entry.fulfilledBy == USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE)
      return;

    entry.fulfilledBy = USER_DATA_EVENT_TYPE::TRADE_LITE;

    // In case this is the new OrderEntry
    entry.clientOrderId = event.clientOrderId;

    entry.symbol = event.symbol;
    entry.orderId = event.orderId;
    entry.side = event.side;
    entry.price = event.price;
    entry.origQty = event.qty;
    entry.lastPrice = event.lastPrice;
    entry.lastQty = event.lastQty;
    entry.eventTime = event.eventTime;
    entry.tradeTime = event.tradeTime;
  });
}

void OrderBook::_updateOrCreateOrderTradeUpdate(
    const UserData::OrderTradeUpdateEvent &event) {
  m_orders->tryEmplace(event.clientOrderId, [&event](OrderEntry &entry) {
    entry.fulfilledBy = USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE;

    // In case this is the new OrderEntry
    entry.clientOrderId = event.clientOrderId;

    entry.symbol = event.symbol;
    entry.orderId = event.orderId;
    entry.side = event.side;
    entry.type = event.orderType;
    entry.positionSide = event.positionSide;
    entry.tif = event.timeInForce;
    entry.executionType = event.executionType;
    entry.orderStatus = event.status;
    entry.origQty = event.origQty;
    entry.executedQty = event.executedQty;
    entry.lastQty = event.lastExecutedQty;
    entry.lastPrice = event.lastPrice;
    entry.price = event.price;
    entry.avgPrice = event.avgPrice;
    entry.commission = event.commission;
    entry.commissionAsset = event.commissionAsset;
    entry.eventTime = event.eventTime;
    entry.tradeTime = event.tradeTime;
    entry.realizedPnL = event.realizedPnL;
    entry.reduceOnly = event.reduceOnly;
  });
}
