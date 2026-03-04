#include "user_data_stream_parser.hpp"
#include "core/controllers/user_data_stream_utils.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/json.hpp"

using namespace UserData;

/* static */ ParsedUserData
UserDataStreamParser::parse(const StreamMessage &msg) {
  if (msg.data.is_empty())
    return ErrorParse{"StreamMessage data is empty"};

  switch (msg.userDataType) {
  case USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE:
    return OrderTradeUpdateParser::parse(msg);
  case USER_DATA_EVENT_TYPE::TRADE_LITE:
    return TradeLiteParser::parse(msg);
  case USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE:
    return AccountUpdateParser::parse(msg);
  case USER_DATA_EVENT_TYPE::MARGIN_CALL:
    return MarginCallParser::parse(msg);
  case USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE:
    return AccountConfigUpdateParser::parse(msg);
  default:
    return ErrorParse{"StreamMessage type is invalid"};
  }
}

/* static */ ParsedUserData
OrderTradeUpdateParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    OrderTradeUpdateEvent data;

    // --- TOP LEVEL TIME ---
  if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  }

  if (auto val = jsonData.get_value(std::string(TRANSACTION_TIME));
      val && val->is_number_unsigned()) {
    data.transactionTime = val->get<uint64_t>();
  }

  auto orderVal = jsonData.get_value(std::string(ORDER));
  if (!orderVal || !orderVal->is_object()) {
    return ErrorParse{"order object is not parsed"};
  }

  JSONQuery orderJson = *orderVal;

  // --- REQUIRED ---
  if (auto val = orderJson.get_value(std::string(ORDER_ID));
      val && val->is_number_unsigned()) {
    data.orderId = val->get<uint64_t>();
  } else {
    return ErrorParse{"orderId is not parsed"};
  }

  if (auto val = orderJson.get_value(std::string(SYMBOL));
      val && val->is_string()) {
    data.symbol = val->get<std::string>();
  } else {
    return ErrorParse{"symbol is not parsed"};
  }

  if (auto val = orderJson.get_value(std::string(CLIENT_ORDER_ID));
      val && val->is_string()) {
    data.clientOrderId = val->get<std::string>();
  } else {
    return ErrorParse{"clientOrderId is not parsed"};
  }

  if (auto val = orderJson.get_value(std::string(STATUS));
      val && val->is_string()) {
    data.status =
        str_to_type(ORDER_STATUS_STR, val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(EXECUTION_TYPE));
      val && val->is_string()) {
    data.executionType =
        str_to_type(EXECUTION_TYPE_STR, val->get_ref<const std::string &>());
  }

  // --- PRICE ---
  if (auto val = orderJson.get_value(std::string(PRICE));
      val && val->is_string()) {
    data.price = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(AVG_PRICE));
      val && val->is_string()) {
    data.avgPrice = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(STOP_PRICE));
      val && val->is_string()) {
    data.stopPrice = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(LAST_PRICE));
      val && val->is_string()) {
    data.lastPrice = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  // --- QTY ---
  if (auto val = orderJson.get_value(std::string(ORIG_QTY));
      val && val->is_string()) {
    data.origQty = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(EXECUTED_QTY));
      val && val->is_string()) {
    data.executedQty = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(LAST_EXECUTED_QTY));
      val && val->is_string()) {
    data.lastExecutedQty =
        Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(CUM_QTY));
      val && val->is_string()) {
    data.cumQuote = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  // --- FEES ---
  if (auto val = orderJson.get_value(std::string(COMMISSION));
      val && val->is_string()) {
    data.commission = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(COMMISSION_ASSET));
      val && val->is_string()) {
    data.commissionAsset = val->get<std::string>();
  }

  // --- PNL ---
  if (auto val = orderJson.get_value(std::string(REALIZED_PNL));
      val && val->is_string()) {
    data.realizedPnL = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  // --- TRADE INFO ---
  if (auto val = orderJson.get_value(std::string(TRADE_ID));
      val && val->is_number_unsigned()) {
    data.tradeId = val->get<uint64_t>();
  }

  if (auto val = orderJson.get_value(std::string(IS_MAKER));
      val && val->is_boolean()) {
    data.isMaker = val->get<bool>();
  }

  if (auto val = orderJson.get_value(std::string(BID_NOTIONAL));
      val && val->is_string()) {
    data.bidNotional = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(ASK_NOTIONAL));
      val && val->is_string()) {
    data.askNotional = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  // --- TYPES ---
  if (auto val = orderJson.get_value(std::string(TIME_IN_FORCE));
      val && val->is_string()) {
    data.timeInForce =
        str_to_type(TIME_IN_FORCE_STR, val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(TYPE));
      val && val->is_string()) {
    data.orderType =
        str_to_type(ORDER_TYPE_STR, val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(ORIG_TYPE));
      val && val->is_string()) {
    data.originalType =
        str_to_type(ORDER_TYPE_STR, val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(SIDE));
      val && val->is_string()) {
    data.side =
        str_to_type(ORDER_SIDE_STR, val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(POSITION_SIDE));
      val && val->is_string()) {
    data.positionSide =
        str_to_type(POSITION_SIDE_STR, val->get_ref<const std::string &>());
  }

  // --- FLAGS ---
  if (auto val = orderJson.get_value(std::string(REDUCE_ONLY));
      val && val->is_boolean()) {
    data.reduceOnly = val->get<bool>();
  }

  if (auto val = orderJson.get_value(std::string(CLOSE_POSITION));
      val && val->is_boolean()) {
    data.closePosition = val->get<bool>();
  }

  if (auto val = orderJson.get_value(std::string(PRICE_PROTECT));
      val && val->is_boolean()) {
    data.priceProtect = val->get<bool>();
  }

  // --- EXTRA ---
  if (auto val = orderJson.get_value(std::string(WORKING_TYPE));
      val && val->is_string()) {
    data.workingType =
        str_to_type(WORKING_TYPE_STR, val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(PRICE_MATCH));
      val && val->is_string()) {
    data.priceMatch = val->get<std::string>();
  }

  if (auto val = orderJson.get_value(std::string(STP_MODE));
      val && val->is_string()) {
    data.selfTradePreventionMode = str_to_type(
        SELF_TRADE_PREVENTION_MODE_STR, val->get_ref<const std::string &>());
  }

  if (auto val = orderJson.get_value(std::string(GOOD_TILL_DATE));
      val && val->is_number_unsigned()) {
    data.goodTillDate = val->get<uint64_t>();
  }

  // --- ORDER TRADE TIME ---
  if (auto val = orderJson.get_value(std::string(TRADE_TIME));
      val && val->is_number_unsigned()) {
    data.tradeTime = val->get<uint64_t>();
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in OrderTradeUpdateParser: " + std::string(e.what())};
  }
}
/* static */ ParsedUserData TradeLiteParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    TradeLiteEvent data;

    if (auto val = jsonData.get_value(std::string(ORDER_ID));
      val && val->is_number_unsigned()) {
    data.orderId = val->get<uint64_t>();
  } else {
    return ErrorParse{"orderId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(TRADE_ID));
      val && val->is_number_unsigned()) {
    data.tradeId = val->get<uint64_t>();
  } else {
    return ErrorParse{"tradeId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(SYMBOL));
      val && val->is_string()) {
    data.symbol = val->get<std::string>();
  } else {
    return ErrorParse{"symbol is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(CLIENT_ORDER_ID));
      val && val->is_string()) {
    data.clientOrderId = val->get<std::string>();
  }

  if (auto val = jsonData.get_value(std::string(SIDE));
      val && val->is_string()) {
    data.side =
        str_to_type(ORDER_SIDE_STR, val->get_ref<const std::string &>());
  }

  if (auto val = jsonData.get_value(std::string(PRICE));
      val && val->is_string()) {
    data.price = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = jsonData.get_value(std::string(QTY));
      val && val->is_string()) {
    data.qty = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = jsonData.get_value(std::string(LAST_PRICE));
      val && val->is_string()) {
    data.lastPrice = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = jsonData.get_value(std::string(LAST_QTY));
      val && val->is_string()) {
    data.lastQty = Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = jsonData.get_value(std::string(MAKER));
      val && val->is_boolean()) {
    data.isMaker = val->get<bool>();
  }

  if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  }

  if (auto val = jsonData.get_value(std::string(TRADE_TIME));
      val && val->is_number_unsigned()) {
    data.tradeTime = val->get<uint64_t>();
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in TradeLiteParser: " + std::string(e.what())};
  }
}

/* static */ ParsedUserData
AccountUpdateParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    AccountUpdateEvent data;

    auto accVal = jsonData.get_value(std::string(ACCOUNT));
  if (!accVal || !accVal->is_object()) {
    return ErrorParse{"account object is not parsed"};
  }

  JSONQuery accJson = *accVal;

  if (auto val = accJson.get_value(std::string(BALANCES));
      val && val->is_array()) {
    for (auto &item : *val) {
      if (!item.is_object())
        continue;

      JSONQuery b{item};
      AccountUpdateEvent::Balance bal;

      if (auto v = b.get_value(std::string(ASSET)); v && v->is_string())
        bal.asset = v->get<std::string>();

      if (auto v = b.get_value(std::string(WALLET_BALANCE));
          v && v->is_string())
        bal.walletBalance =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = b.get_value(std::string(CROSS_WALLET_BALANCE));
          v && v->is_string())
        bal.crossWalletBalance =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = b.get_value(std::string(BALANCE_CHANGE));
          v && v->is_string())
        bal.balanceChange =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      data.balances.push_back(std::move(bal));
    }
  }

  if (auto val = accJson.get_value(std::string(POSITIONS));
      val && val->is_array()) {
    for (auto &item : *val) {
      if (!item.is_object())
        continue;

      JSONQuery p{item};
      AccountUpdateEvent::Position pos;

      if (auto v = p.get_value(std::string(SYMBOL)); v && v->is_string())
        pos.symbol = v->get<std::string>();

      if (auto v = p.get_value(std::string(POSITION_AMT)); v && v->is_string())
        pos.positionAmt =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(ENTRY_PRICE)); v && v->is_string())
        pos.entryPrice = Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(REALIZED_PNL)); v && v->is_string())
        pos.realizedPnL =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(UNREALIZED_PNL));
          v && v->is_string())
        pos.unrealizedPnL =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(ISOLATED_WALLET));
          v && v->is_string())
        pos.isolatedWallet =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(POSITION_SIDE)); v && v->is_string())
        pos.positionSide =
            str_to_type(POSITION_SIDE_STR, v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(MARGIN_ASSET)); v && v->is_string())
        pos.marginAsset = v->get<std::string>();

      if (auto v = p.get_value(std::string(BREAK_EVEN_PRICE));
          v && v->is_string())
        pos.breakEvenPrice =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      data.positions.push_back(std::move(pos));
    }
  }

  if (auto val = accJson.get_value(std::string(REASON));
      val && val->is_string()) {
    data.reason = val->get<std::string>();
  }

  if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  }

  if (auto val = jsonData.get_value(std::string(TRANSACTION_TIME));
      val && val->is_number_unsigned()) {
    data.transactionTime = val->get<uint64_t>();
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in AccountUpdateParser: " + std::string(e.what())};
  }
}

/* static */ ParsedUserData MarginCallParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    MarginCallEvent data;

    if (auto val = jsonData.get_value(std::string(CROSS_WALLET));
      val && val->is_string()) {
    data.crossWalletBalance =
        Fixed::str_to_fixed(val->get_ref<const std::string &>());
  }

  if (auto val = jsonData.get_value(std::string(POSITIONS));
      val && val->is_array()) {
    for (auto &item : *val) {
      if (!item.is_object())
        continue;

      JSONQuery p{item};
      MarginCallEvent::Position pos;

      if (auto v = p.get_value(std::string(SYMBOL)); v && v->is_string())
        pos.symbol = v->get<std::string>();

      if (auto v = p.get_value(std::string(POSITION_SIDE)); v && v->is_string())
        pos.positionSide =
            str_to_type(POSITION_SIDE_STR, v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(POSITION_AMT)); v && v->is_string())
        pos.positionAmt =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(MARGIN_TYPE)); v && v->is_string())
        pos.marginType = v->get<std::string>();

      if (auto v = p.get_value(std::string(ISOLATED_WALLET));
          v && v->is_string())
        pos.isolatedWallet =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(MARK_PRICE)); v && v->is_string())
        pos.markPrice = Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(UNREALIZED_PNL));
          v && v->is_string())
        pos.unrealizedPnL =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      if (auto v = p.get_value(std::string(MAINT_MARGIN)); v && v->is_string())
        pos.maintenanceMargin =
            Fixed::str_to_fixed(v->get_ref<const std::string &>());

      data.positions.push_back(std::move(pos));
    }
  }

  if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in MarginCallParser: " + std::string(e.what())};
  }
}

/* static */ ParsedUserData
AccountConfigUpdateParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    AccountConfigUpdateEvent data;

    if (auto val = jsonData.get_value(std::string(LEVERAGE_OBJ));
      val && val->is_object()) {

    JSONQuery obj = *val;
    AccountConfigUpdateEvent::LeverageUpdate upd;

    if (auto v = obj.get_value(std::string(SYMBOL)); v && v->is_string())
      upd.symbol = v->get<std::string>();

    if (auto v = obj.get_value(std::string(LEVERAGE));
        v && v->is_number_unsigned())
      upd.leverage = v->get<uint32_t>();

    data.leverageUpdate = std::move(upd);
  }

  if (auto val = jsonData.get_value(std::string(MULTI_ASSET_OBJ));
      val && val->is_object()) {

    JSONQuery obj = *val;
    AccountConfigUpdateEvent::MultiAssetModeUpdate upd;

    if (auto v = obj.get_value(std::string(MULTI_ASSETS_MODE));
        v && v->is_boolean())
      upd.multiAssetsMode = v->get<bool>();

    data.multiAssetModeUpdate = std::move(upd);
  }

  if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned())
    data.eventTime = val->get<uint64_t>();

  if (auto val = jsonData.get_value(std::string(TRANSACTION_TIME));
      val && val->is_number_unsigned())
    data.transactionTime = val->get<uint64_t>();

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in AccountConfigUpdateParser: " + std::string(e.what())};
  }
}
