#include "market_data_controller.hpp"

#include "core/controllers/market_data_utils.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/stream/market_stream.hpp"
#include "core/stream/stream.hpp"
#include "core/utils/json.hpp"
#include "core/utils/queue.hpp"
#include "core/utils/thread.hpp"

#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

using namespace Market;

MarketDataController::MarketDataController()
    : m_queryBuilder(std::make_unique<MarketStreamQueryBuilder>(
          MARKET_STREAM_METHOD::SUBSCRIBE)),
      m_parsedStreamData(std::make_unique<Queue<ParsedMarketData>>()),
      m_listenThread(std::make_unique<Thread>()),
      m_readThread(std::make_unique<Thread>()),
      m_marketMsgQueues(std::make_unique<MessageStreams>()) {
  // Market Stream init
  m_marketStream = std::make_unique<MarketStream>(m_marketMsgQueues->msgQueue);

  NetError wsErr = m_marketStream->connect_to_websocket();
  if (!wsErr.hasError()) {
    _start_listen_thread();
    _start_read_thread();
  }

  m_marketMsgQueues->resultQueue.register_callback(
      [this](const ResultMessage &msg) { _fulfill_pending_result(msg); });

  m_marketMsgQueues->streamQueue.register_callback(
      [this](const StreamMessage &msg) {
        m_parsedStreamData->push_message(MarketDataParser::parse(msg));
      });
};

std::vector<MarketRequest> SubscriptionsList::get_list() const {
  std::lock_guard<std::mutex> lock(m_mtx);
  return m_list;
}

void SubscriptionsList::add_to_list(const MarketRequest &req) {
  std::lock_guard<std::mutex> lock(m_mtx);
  m_list.push_back(req);
}

void SubscriptionsList::remove_from_list(const MarketRequest &request) {
  std::lock_guard<std::mutex> lock(m_mtx);
  auto it =
      std::find_if(m_list.begin(), m_list.end(),
                   [&request](const MarketRequest &r) { return r == request; });

  if (it != m_list.end()) {
    m_list.erase(it);
  }
}

bool MarketDataController::subscribe_to(const MarketRequest &req) {
  m_queryBuilder->setMethod(MARKET_STREAM_METHOD::SUBSCRIBE);

  switch (req.type) {
  case MARKET_DATA_TYPE::TRADE:
    m_queryBuilder->add_trade_symbol(req.symbol);
    break;
  case MARKET_DATA_TYPE::AGG_TRADE:
    m_queryBuilder->add_aggTrade_symbol(req.symbol);
    break;
  case MARKET_DATA_TYPE::MARK_PRICE:
    m_queryBuilder->add_markPrice_symbol(req.symbol, req.fast_update);
    break;
  case MARKET_DATA_TYPE::DIFF_DEPTH:
    m_queryBuilder->add_diffDepth_symbol(req.symbol, req.fast_update);
    break;
  case MARKET_DATA_TYPE::PART_DEPTH:
    m_queryBuilder->add_partDepth_symbol(req.symbol, req.depth_levels,
                                         req.fast_update);
    break;
  case MARKET_DATA_TYPE::BOOK_TICKER:
    m_queryBuilder->add_bookTicker_symbol(req.symbol);
    break;
  default:
    return false;
  }

  std::optional<JSONQuery> query = m_queryBuilder->commit();

  if (query) {
    auto value =
        query.value().get_value(std::string(MarketStreamQueryBuilder::ID));

    std::string id_str = (value ? value->get<std::string>() : "");

    {
      std::lock_guard<std::mutex> lock(m_pendingReqMtx);
      m_pendingReq.emplace(
          id_str, std::make_pair(req, MARKET_STREAM_METHOD::SUBSCRIBE));
    }

    NetError netErr = m_marketStream->execute_query(query.value());
    if (!netErr.hasError()) {
      return true;
    }
  }

  return false;
}

bool MarketDataController::unsubscribe_from(const MarketRequest &req) {
  m_queryBuilder->setMethod(MARKET_STREAM_METHOD::UNSUBSCRIBE);

  switch (req.type) {
  case MARKET_DATA_TYPE::TRADE:
    m_queryBuilder->add_trade_symbol(req.symbol);
    break;
  case MARKET_DATA_TYPE::AGG_TRADE:
    m_queryBuilder->add_aggTrade_symbol(req.symbol);
    break;
  case MARKET_DATA_TYPE::MARK_PRICE:
    m_queryBuilder->add_markPrice_symbol(req.symbol, req.fast_update);
    break;
  case MARKET_DATA_TYPE::DIFF_DEPTH:
    m_queryBuilder->add_diffDepth_symbol(req.symbol, req.fast_update);
    break;
  case MARKET_DATA_TYPE::PART_DEPTH:
    m_queryBuilder->add_partDepth_symbol(req.symbol, req.depth_levels,
                                         req.fast_update);
    break;
  case MARKET_DATA_TYPE::BOOK_TICKER:
    m_queryBuilder->add_bookTicker_symbol(req.symbol);
    break;
  default:
    return false;
  }

  std::optional<JSONQuery> query = m_queryBuilder->commit();

  if (query) {
    auto value =
        query.value().get_value(std::string(MarketStreamQueryBuilder::ID));

    std::string id_str = (value ? value->get<std::string>() : "");

    {
      std::lock_guard<std::mutex> lock(m_pendingReqMtx);
      m_pendingReq.emplace(
          id_str, std::make_pair(req, MARKET_STREAM_METHOD::UNSUBSCRIBE));
    }

    NetError netErr = m_marketStream->execute_query(query.value());
    if (!netErr.hasError()) {
      return true;
    }
  }

  return false;
}

std::vector<MarketRequest> MarketDataController::get_list_of_subscriptions() {
  return m_subList.get_list();
}

void MarketDataController::_start_listen_thread() {
  m_listenThread->start(&MarketStream::start_listening, m_marketStream.get());
}
void MarketDataController::_start_read_thread() {
  m_readThread->start(&MarketDataController::_start_buffer_reading, this);
}

void MarketDataController::_start_buffer_reading() {
  while (true) {
    std::string out_msg;
    bool res = m_marketMsgQueues->msgQueue.pop_message(out_msg);
    if (res) {
      Log::log("MARKETDATA" + out_msg);

      _parse_msg(std::move(out_msg));
    }
  }
}

MESSAGE_TYPE MarketDataController::_detect_type(const JSONQuery &msg) {
  if (msg.is_key_exists(std::string(MsgKeys::STREAM)))
    return MESSAGE_TYPE::STREAM;

  if (msg.is_key_exists(std::string(MsgKeys::RESULT)))
    return MESSAGE_TYPE::RESULT;

  if (msg.is_key_exists(std::string(MsgKeys::ERROR)))
    return MESSAGE_TYPE::ERROR;

  return MESSAGE_TYPE::UNKNOWN;
}

void MarketDataController::_fulfill_pending_result(const ResultMessage &msg) {
  std::lock_guard<std::mutex> lock(m_pendingReqMtx);

  auto it = m_pendingReq.find(msg.reqId);
  if (it == m_pendingReq.end())
    return;

  if (msg.isSuccessResult) {
    if (it->second.second == MARKET_STREAM_METHOD::SUBSCRIBE)
      m_subList.add_to_list(it->second.first);
    else if (it->second.second == MARKET_STREAM_METHOD::UNSUBSCRIBE)
      m_subList.remove_from_list(it->second.first);
  }

  m_pendingReq.erase(it);
}

void MarketDataController::_parse_msg(const std::string &&msg) {
  JSONQuery jsonMsg(msg);
  MESSAGE_TYPE msg_type = _detect_type(jsonMsg);

  switch (msg_type) {
  case MESSAGE_TYPE::STREAM: {
    auto keyVal = jsonMsg.get_value(std::string(MsgKeys::STREAM));
    auto value = jsonMsg.get_value(std::string(MsgKeys::STREAM_DATA));

    std::string keyStr =
        (keyVal && keyVal->is_string() ? keyVal->get<std::string>()
                                       : std::string(MsgKeys::DEFAULT_VAL));
    JSONQuery jsonData =
        (value && value->is_object() ? JSONQuery{value.value()} : JSONQuery{});

    StreamMessage msg{keyStr, jsonData};
    m_marketMsgQueues->streamQueue.push_message(std::move(msg));
    break;
  }
  case MESSAGE_TYPE::RESULT: {
    auto keyVal = jsonMsg.get_value(std::string(MsgKeys::RESULT));
    auto value = jsonMsg.get_value(std::string(MsgKeys::RESULT_DATA));

    std::string keyStr{std::string(MsgKeys::DEFAULT_VAL)};

    if (keyVal && keyVal->is_string()) {
      keyStr = keyVal->get<std::string>();
    } else if (keyVal && keyVal->is_null()) {
      keyStr = "null";
    }

    std::string idStr =
        (value && value->is_string() ? value->get<std::string>()
                                     : std::string(MsgKeys::DEFAULT_VAL));

    ResultMessage msg{keyStr, idStr};

    m_marketMsgQueues->resultQueue.push_message(std::move(msg));
    break;
  }
  case MESSAGE_TYPE::ERROR: {
    auto keyVal = jsonMsg.get_value(std::string(MsgKeys::ERROR));
    auto value = jsonMsg.get_value(std::string(MsgKeys::ERROR_DATA));

    std::string keyStr =
        (keyVal && keyVal->is_string() ? keyVal->get<std::string>()
                                       : std::string(MsgKeys::DEFAULT_VAL));
    std::string errMsg =
        (value && value->is_string() ? value->get<std::string>()
                                     : std::string(MsgKeys::DEFAULT_VAL));

    ErrorMessage msg{keyStr, errMsg};
    m_marketMsgQueues->errorQueue.push_message(std::move(msg));
    break;
  }
  // TODO: add handlers for UNKNOWN messages
  case MESSAGE_TYPE::UNKNOWN: {
    UnknownMessage msg{jsonMsg};
    m_marketMsgQueues->unknownQueue.push_message(std::move(msg));
    break;
  }
  }
}
