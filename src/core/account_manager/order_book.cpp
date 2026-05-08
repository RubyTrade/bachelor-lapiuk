#include "core/account_manager/order_book.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/parsers/user_data_stream_parser.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/helper_utils.hpp"
#include "core/utils/log.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <variant>

namespace {
std::string classify_trade_kind(const ORDER_TYPE type) {
  switch (type) {
  case ORDER_TYPE::TAKE_PROFIT:
  case ORDER_TYPE::TAKE_PROFIT_MARKET:
  case ORDER_TYPE::STOP:
  case ORDER_TYPE::STOP_MARKET:
  case ORDER_TYPE::TRAILING_STOP_MARKET:
    return "TP/SL";
  case ORDER_TYPE::LIMIT:
  case ORDER_TYPE::MARKET:
    return "REGULAR";
  default:
    return "UNKNOWN";
  }
}
} // namespace

OrderBook::OrderBook()
    : m_orders(std::make_unique<AccountOrders>()),
      m_eventQueue(std::make_unique<Queue<UserData::ParsedUserData>>()),
      m_processingThread(std::make_unique<Thread>()) {
  _updateLastUpdateTime();
}

OrderBook::~OrderBook() { stop(); }

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

void OrderBook::enqueue(UserData::ParsedUserData event) {
  if (!std::holds_alternative<ErrorParse>(event))
    m_eventQueue->push_message(std::move(event));

  return;
}

void OrderBook::start() {
  m_isQueueActive = true;
  m_processingThread->start(&OrderBook::_listenToUpdates, this);
}

void OrderBook::stop() {
  m_isQueueActive = false;
  m_eventQueue->stop_queue();
  m_processingThread->stop();
}

void OrderBook::_listenToUpdates() {
  while (m_isQueueActive) {
    UserData::ParsedUserData out_msg;
    if (!m_eventQueue->pop_message(out_msg))
      break;

    updateOrCreateOrder(std::move(out_msg));
  }
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
  if (m_update_cb)
    m_update_cb();
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
  m_orders->tryEmplace(event.clientOrderId, [&event, this](OrderEntry &entry) {
    const bool isNewEntry = entry.clientOrderId.empty();
    const bool isAlgoUpdate = event.eventType == USER_DATA_EVENT_TYPE::ALGO_UPDATE;
    const bool algoStateChanged =
        isNewEntry ||
        entry.orderStatus != event.status ||
        entry.executionType != event.executionType;

    const bool becameFilled =
        entry.orderStatus != ORDER_STATUS::FILLED &&
        event.executionType == EXECUTION_TYPE::TRADE &&
        event.status == ORDER_STATUS::FILLED;

    entry.fulfilledBy = event.eventType;

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

    if (isAlgoUpdate && algoStateChanged) {
      _logAlgoUpdate(entry);
    }

    if (becameFilled) {
      _logFilledTrade(entry);
    }
  });
}

std::string OrderBook::_formatFilledTradeLog(const OrderEntry &entry) const {
  std::ostringstream ss;
  const std::string orderType = type_to_str(ORDER_TYPE_STR, entry.type);
  const std::string tradeKind = classify_trade_kind(entry.type);

  ss << "\n========== TRADE FILLED ==========\n"
     << "  Symbol    : " << entry.symbol << "\n"
     << "  Side      : " << type_to_str(ORDER_SIDE_STR, entry.side) << " | "
     << type_to_str(POSITION_SIDE_STR, entry.positionSide) << "\n"
     << "  Kind      : " << tradeKind << " (" << orderType << ")\n"
     << "  Avg / Qty : " << entry.avgPrice.to_string() << " / "
     << entry.executedQty.to_string() << "\n"
     << "  PnL       : " << entry.realizedPnL.to_string() << "\n"
     << "  Fee       : " << entry.commission.to_string();

  if (!entry.commissionAsset.empty()) {
    ss << " " << entry.commissionAsset;
  }

  ss << "\n"
     << "  ClientId  : " << entry.clientOrderId << "\n"
     << "==================================";
  return ss.str();
}

void OrderBook::_logFilledTrade(const OrderEntry &entry) const {
  Log::log_info(_formatFilledTradeLog(entry));
}

std::string OrderBook::_formatAlgoUpdateLog(const OrderEntry &entry) const {
  std::ostringstream ss;
  const std::string orderType = type_to_str(ORDER_TYPE_STR, entry.type);
  const std::string tradeKind = classify_trade_kind(entry.type);

  ss << "\n----------- ALGO UPDATE -----------\n"
     << "  Symbol    : " << entry.symbol << "\n"
     << "  Side      : " << type_to_str(ORDER_SIDE_STR, entry.side) << " | "
     << type_to_str(POSITION_SIDE_STR, entry.positionSide) << "\n"
     << "  Kind      : " << tradeKind << " (" << orderType << ")\n"
     << "  Status    : " << type_to_str(ORDER_STATUS_STR, entry.orderStatus)
     << " / " << type_to_str(EXECUTION_TYPE_STR, entry.executionType) << "\n"
     << "  OrderPx   : " << entry.price.to_string() << "\n"
     << "  Filled    : " << entry.executedQty.to_string()
     << " @ " << entry.avgPrice.to_string() << "\n"
     << "  ClientId  : " << entry.clientOrderId << "\n"
     << "-----------------------------------";
  return ss.str();
}

void OrderBook::_logAlgoUpdate(const OrderEntry &entry) const {
  Log::log_info(_formatAlgoUpdateLog(entry));
}
