#include "trading_stream_controller.hpp"
#include "core/controllers/trading_stream_utils.hpp"
#include "core/net/net.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

using namespace Trading;

TradingStreamController::TradingStreamController()
    : m_queryBuilder(std::make_unique<TradingStreamQueryBuilder>(
          TRADE_STREAM_METHOD::INVALID_METHOD)),
      m_paramBuilder(std::make_unique<ParametersBuilder>()),
      m_listenThread(std::make_unique<Thread>()),
      m_readThread(std::make_unique<Thread>()) {
  // Trade Stream init
  m_tradeStream = std::make_unique<TradingStream>(m_msgQueue);

  NetError wsErr = m_tradeStream->connect_to_websocket();
  if (!wsErr.hasError()) {
    _start_listen_thread();
    _start_read_thread();
  }

  m_resultsQueue.register_callback(
      [this](const ResultMessage &msg) { _fulfill_pending_result(msg); });

  _do_session_logon();
};

NetError TradingStreamController::_do_session_logon() {
  NetError netErr;
  m_queryBuilder->setMethod(TRADE_STREAM_METHOD::SESSION_LOGON);
  std::optional<JSONQuery> auth_query = m_queryBuilder->commit();

  if (auth_query) {
    netErr = m_tradeStream->execute_query(auth_query.value());
  }

  return netErr;
}

void TradingStreamController::_start_listen_thread() {
  m_listenThread->start(&TradingStream::start_listening, m_tradeStream.get());
}
void TradingStreamController::_start_read_thread() {
  m_readThread->start(&TradingStreamController::_start_buffer_reading, this);
}

void TradingStreamController::_start_buffer_reading() {
  while (true) {
    std::string out_msg;
    bool res = m_msgQueue.pop_message(out_msg);
    if (res) {
      Log::log("TRADINGDATA: " + out_msg);

      _parse_msg(std::move(out_msg));
    }
  }
}

void TradingStreamController::_fulfill_pending_result(
    const ResultMessage &msg) {
  std::lock_guard<std::mutex> lock(m_pendingReqMtx);

  auto it = m_pendingRequests.find(msg.id);
  if (it == m_pendingRequests.end())
    return;

  if (msg.isSuccess()) {
    // TODO: handle the error and results
  }

  m_pendingRequests.erase(it);
}

void TradingStreamController::_parse_msg(const std::string &&msg) {
  JSONQuery jsonMsg(msg);

  auto id = jsonMsg.get_value(std::string(MsgKeys::ID));
  auto status = jsonMsg.get_value(std::string(MsgKeys::STATUS));

  std::string idStr =
      (id && id->is_string() ? id->get<std::string>()
                             : std::string(MsgKeys::DEFAULT_VAL));

  uint16_t statusCode =
      (status && status->is_number_unsigned() ? status->get<uint16_t>() : 0);

  ResultMessage res{idStr, statusCode};

  if (res.isSuccess()) {
    auto keyVal = jsonMsg.get_value(std::string(MsgKeys::RESULT));

    if (!keyVal)
      return;

    JSONQuery resJson{};

    if (keyVal && keyVal->is_object()) {
      resJson = {keyVal->get<nlohmann::json>()};
    }

    res.result_msg = resJson;
  } else {
    auto keyVal = jsonMsg.get_value(std::string(MsgKeys::ERROR));

    if (!keyVal)
      return;

    JSONQuery errJson{};

    if (keyVal && keyVal->is_object()) {
      errJson = {keyVal->get<nlohmann::json>()};
    }

    if (errJson.is_empty())
      return;

    auto codeVal = errJson.get_value(std::string(MsgKeys::CODE));
    int errCode = (codeVal && codeVal->is_number() ? status->get<int>() : 0);

    auto errMsgVal = errJson.get_value(std::string(MsgKeys::MSG));
    std::string errMsgStr = (errMsgVal && errMsgVal->is_string()
                                 ? status->get<std::string>()
                                 : std::string(MsgKeys::DEFAULT_VAL));

    res.error_code = errCode;
    res.error_msg = errMsgStr;
  }

  m_resultsQueue.push_message(std::move(res));
}
/*
std::vector<TradeRequest> RequestsList::get_list() const {
  std::lock_guard<std::mutex> lock(m_mtx);
  return m_list;
}

void RequestsList::add_to_list(const TradeRequest &req) {
  std::lock_guard<std::mutex> lock(m_mtx);
  m_list.push_back(req);
}

void RequestsList::remove_from_list(const TradeRequest &request) {
  std::lock_guard<std::mutex> lock(m_mtx);
  auto it =
      std::find_if(m_list.begin(), m_list.end(),
                   [&request](const TradeRequest &r) { return r == request;
});

  if (it != m_list.end()) {
    m_list.erase(it);
  }
}
*/

std::string TradingStreamController::create_order(const TradeRequest &req) {
  m_queryBuilder->setMethod(TRADE_STREAM_METHOD::ORDER_PLACE);

  if (req.symbol.empty() || req.quantity < Fixed{0, req.quantity.scale()} ||
      req.price < Fixed{0, req.price.scale()})
    return {};

  m_paramBuilder->add_side(req.order_side);
  m_paramBuilder->add_positionSide(req.position_side);
  m_paramBuilder->add_symbol(req.symbol);
  m_paramBuilder->add_type(req.order_type);
  m_paramBuilder->add_quantity(req.quantity);

  if (req.order_type == ORDER_TYPE::LIMIT ||
      req.order_type == ORDER_TYPE::STOP ||
      req.order_type == ORDER_TYPE::TAKE_PROFIT) {
    m_paramBuilder->add_price(req.price);
    m_paramBuilder->add_timeInForce(req.timeInForce);
  }

  if (req.reduceOnly) {
    m_paramBuilder->add_reduceOnly(req.reduceOnly);
  }

  m_paramBuilder->add_clientOrderId(req.getClientOrderId());

  std::optional<JSONQuery> param_query = m_paramBuilder->commit();

  if (!param_query) {
    m_queryBuilder->cleanup();
    return {};
  }

  std::optional<JSONQuery> query =
      m_queryBuilder->add_borderless_params(param_query.value()).commit();

  if (query) {
    NetError netErr = m_tradeStream->execute_query(query.value());

    {
      std::lock_guard<std::mutex> lock(m_pendingReqMtx);
      m_pendingRequests.emplace(
          req.getClientOrderId(),
          std::make_pair(req, TRADE_STREAM_METHOD::ORDER_PLACE));
    }

    if (!netErr.hasError()) {
      return req.getClientOrderId();
    }
  }

  return {};
}

bool TradingStreamController::cancel_order(const TradeRequest &req) {
  m_queryBuilder->setMethod(TRADE_STREAM_METHOD::ORDER_CANCEL);

  // TODO: check if this order is available for cancel
  m_paramBuilder->add_origClientOrderId(req.getClientOrderId());
  m_paramBuilder->add_symbol(req.symbol);

  std::optional<JSONQuery> param_query = m_paramBuilder->commit();

  if (!param_query) {
    m_queryBuilder->cleanup();
    return false;
  }

  std::optional<JSONQuery> query =
      m_queryBuilder->add_borderless_params(param_query.value()).commit();

  if (query) {
    NetError netErr = m_tradeStream->execute_query(query.value());

    Log::log(query->str());

    {
      std::lock_guard<std::mutex> lock(m_pendingReqMtx);
      m_pendingRequests.emplace(
          req.getClientOrderId(),
          std::make_pair(req, TRADE_STREAM_METHOD::ORDER_CANCEL));
    }

    if (!netErr.hasError()) {
      return true;
    }
  }

  return false;
}

bool TradingStreamController::get_order_status(const TradeRequest &req) {
  m_queryBuilder->setMethod(TRADE_STREAM_METHOD::ORDER_STATUS);

  // TODO: check if this order is available for status
  m_paramBuilder->add_origClientOrderId(req.getClientOrderId());
  m_paramBuilder->add_symbol(req.symbol);

  std::optional<JSONQuery> param_query = m_paramBuilder->commit();

  if (!param_query) {
    m_queryBuilder->cleanup();
    return false;
  }

  std::optional<JSONQuery> query =
      m_queryBuilder->add_borderless_params(param_query.value()).commit();

  if (query) {
    NetError netErr = m_tradeStream->execute_query(query.value());

    {
      std::lock_guard<std::mutex> lock(m_pendingReqMtx);
      m_pendingRequests.emplace(
          req.getClientOrderId(),
          std::make_pair(req, TRADE_STREAM_METHOD::ORDER_STATUS));
    }

    if (!netErr.hasError()) {
      return true;
    }
  }

  return false;
}
