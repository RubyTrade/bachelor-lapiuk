#include "trading_stream_parser.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/utils/helper_utils.hpp"
#include "core/utils/json.hpp"

using namespace Trading;

ParsedTradingStream TradingStreamParser::parse(const TradingResultStream &msg) {
  if (!msg.result.isSuccess()) {
    ErrorParse error;
    error.parse_error = msg.result.error_msg;
    return error;
  }

  switch (msg.type) {
  case TRADE_STREAM_METHOD::ACCOUNT_STATUS:
    return AccountStatusParser::parse(msg.result);
  case TRADE_STREAM_METHOD::ACCOUNT_BALANCE:
    return AccountBalanceParser::parse(msg.result);
  case TRADE_STREAM_METHOD::ORDER_PLACE:
    return OrderPlaceParser::parse(msg.result);
  case TRADE_STREAM_METHOD::ORDER_MODIFY:
    return OrderModifyParser::parse(msg.result);
  case TRADE_STREAM_METHOD::ORDER_CANCEL:
    return OrderCancelParser::parse(msg.result);
  case TRADE_STREAM_METHOD::ORDER_STATUS:
    return OrderStatusParser::parse(msg.result);
  case TRADE_STREAM_METHOD::ACCOUNT_POSITION:
    return AccountPositionParser::parse(msg.result);
  default:
    ErrorParse error;
    error.parse_error = "Unknown TRADE_STREAM_METHOD";
    return error;
  }
}

// ============================================================================
// Account Status Parser
// ============================================================================
ParsedTradingStream AccountStatusParser::parse(const ResultMessage &msg) {
  try {
    AccountStatusResponse response;

    const JSONQuery &json = msg.result_msg;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    if (auto val = json.get_value(std::string(CAN_TRADE));
        val && val->is_boolean()) {
      response.canTrade = val->get<bool>();
    }

    if (auto val = json.get_value(std::string(CAN_DEPOSIT));
        val && val->is_boolean()) {
      response.canDeposit = val->get<bool>();
    }

    if (auto val = json.get_value(std::string(CAN_WITHDRAW));
        val && val->is_boolean()) {
      response.canWithdraw = val->get<bool>();
    }

    if (auto val = json.get_value(std::string(UPDATE_TIME));
        val && val->is_number_unsigned()) {
      response.updateTime = val->get<uint64_t>();
    }

    return response;
  } catch (const nlohmann::json::exception &e) {
    ErrorParse error;
    error.parse_error = std::string("AccountStatusParser error: ") + e.what();
    return error;
  }
}

// ============================================================================
// Account Balance Parser
// ============================================================================
ParsedTradingStream AccountBalanceParser::parse(const ResultMessage &msg) {
  try {
    AccountBalanceResponse response;

    const JSONQuery &json = msg.result_msg;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    // Check if result is array
    if (!json.is_array()) {
      ErrorParse error;
      error.parse_error = "AccountBalanceParser: expected array";
      return error;
    }

    auto balancesArray = json.get_array();
    if (!balancesArray) {
      ErrorParse error;
      error.parse_error = "AccountBalanceParser: failed to get array";
      return error;
    }

    for (const auto &balanceJson : *balancesArray) {
      JSONQuery balanceQuery(balanceJson);

      AccountBalanceResponse::Balance balance;

      if (auto val = balanceQuery.get_value(std::string(ASSET));
          val && val->is_string()) {
        balance.asset = val->get<std::string>();
      }

      if (auto val = balanceQuery.get_value(std::string(BALANCE));
          val && val->is_string()) {
        balance.balance =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceQuery.get_value(std::string(CROSS_WALLET_BALANCE));
          val && val->is_string()) {
        balance.crossWalletBalance =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceQuery.get_value(std::string(CROSS_UN_PNL));
          val && val->is_string()) {
        balance.crossUnPnl =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceQuery.get_value(std::string(AVAILABLE_BALANCE));
          val && val->is_string()) {
        balance.availableBalance =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceQuery.get_value(std::string(MAX_WITHDRAW_AMOUNT));
          val && val->is_string()) {
        balance.maxWithdrawAmount =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = balanceQuery.get_value(std::string(MARGIN_AVAILABLE));
          val && val->is_boolean()) {
        balance.marginAvailable = val->get<bool>();
      }

      if (auto val = balanceQuery.get_value(std::string(UPDATE_TIME));
          val && val->is_number_unsigned()) {
        balance.updateTime = val->get<uint64_t>();
      }

      response.balances.push_back(balance);
    }

    return response;
  } catch (const nlohmann::json::exception &e) {
    ErrorParse error;
    error.parse_error = std::string("AccountBalanceParser error: ") + e.what();
    return error;
  }
}

// ============================================================================
// Order Place Parser
// ============================================================================
ParsedTradingStream OrderPlaceParser::parse(const ResultMessage &msg) {
  try {
    OrderPlaceResponse response;

    const JSONQuery &json = msg.result_msg;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    if (auto val = json.get_value(std::string(SYMBOL));
        val && val->is_string()) {
      response.symbol = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(ORDER_ID));
        val && val->is_number_integer()) {
      response.orderId = val->get<int64_t>();
    }

    if (auto val = json.get_value(std::string(CLIENT_ORDER_ID));
        val && val->is_string()) {
      response.clientOrderId = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(PRICE));
        val && val->is_string()) {
      response.price = Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(ORIG_QTY));
        val && val->is_string()) {
      response.origQty =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(EXECUTED_QTY));
        val && val->is_string()) {
      response.executedQty =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(CUM_QUOTE));
        val && val->is_string()) {
      response.cumQuote =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(STATUS));
        val && val->is_string()) {
      response.status = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(UPDATE_TIME));
        val && val->is_number_unsigned()) {
      response.updateTime = val->get<uint64_t>();
    }

    if (auto val = json.get_value(std::string(WORKING_TIME));
        val && val->is_number_unsigned()) {
      response.workingTime = val->get<uint64_t>();
    }

    // Parse enums
    if (auto val = json.get_value(std::string(TYPE)); val && val->is_string()) {
      response.type = str_to_type(ORDER_TYPE_STR, val->get<std::string>());
    }

    if (auto val = json.get_value(std::string(SIDE)); val && val->is_string()) {
      response.side = str_to_type(ORDER_SIDE_STR, val->get<std::string>());
    }

    if (auto val = json.get_value(std::string(POSITION_SIDE));
        val && val->is_string()) {
      response.positionSide =
          str_to_type(POSITION_SIDE_STR, val->get<std::string>());
    }

    if (auto val = json.get_value(std::string(TIME_IN_FORCE));
        val && val->is_string()) {
      response.timeInForce =
          str_to_type(TIME_IN_FORCE_STR, val->get<std::string>());
    }

    return response;
  } catch (const nlohmann::json::exception &e) {
    ErrorParse error;
    error.parse_error = std::string("OrderPlaceParser error: ") + e.what();
    return error;
  }
}

// ============================================================================
// Order Modify Parser
// ============================================================================
ParsedTradingStream OrderModifyParser::parse(const ResultMessage &msg) {
  try {
    OrderModifyResponse response;

    const JSONQuery &json = msg.result_msg;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    if (auto val = json.get_value(std::string(SYMBOL));
        val && val->is_string()) {
      response.symbol = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(ORDER_ID));
        val && val->is_number_integer()) {
      response.orderId = val->get<int64_t>();
    }

    if (auto val = json.get_value(std::string(CLIENT_ORDER_ID));
        val && val->is_string()) {
      response.clientOrderId = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(PRICE));
        val && val->is_string()) {
      response.price = Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(ORIG_QTY));
        val && val->is_string()) {
      response.origQty =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(STATUS));
        val && val->is_string()) {
      response.status = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(UPDATE_TIME));
        val && val->is_number_unsigned()) {
      response.updateTime = val->get<uint64_t>();
    }

    return response;
  } catch (const nlohmann::json::exception &e) {
    ErrorParse error;
    error.parse_error = std::string("OrderModifyParser error: ") + e.what();
    return error;
  }
}

// ============================================================================
// Order Cancel Parser
// ============================================================================
ParsedTradingStream OrderCancelParser::parse(const ResultMessage &msg) {
  try {
    OrderCancelResponse response;

    const JSONQuery &json = msg.result_msg;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    if (auto val = json.get_value(std::string(SYMBOL));
        val && val->is_string()) {
      response.symbol = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(ORDER_ID));
        val && val->is_number_integer()) {
      response.orderId = val->get<int64_t>();
    }

    if (auto val = json.get_value(std::string(CLIENT_ORDER_ID));
        val && val->is_string()) {
      response.clientOrderId = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(STATUS));
        val && val->is_string()) {
      response.status = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(UPDATE_TIME));
        val && val->is_number_unsigned()) {
      response.updateTime = val->get<uint64_t>();
    }

    return response;
  } catch (const nlohmann::json::exception &e) {
    ErrorParse error;
    error.parse_error = std::string("OrderCancelParser error: ") + e.what();
    return error;
  }
}

// ============================================================================
// Order Status Parser
// ============================================================================
ParsedTradingStream OrderStatusParser::parse(const ResultMessage &msg) {
  try {
    OrderStatusResponse response;

    const JSONQuery &json = msg.result_msg;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    if (auto val = json.get_value(std::string(SYMBOL));
        val && val->is_string()) {
      response.symbol = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(ORDER_ID));
        val && val->is_number_integer()) {
      response.orderId = val->get<int64_t>();
    }

    if (auto val = json.get_value(std::string(CLIENT_ORDER_ID));
        val && val->is_string()) {
      response.clientOrderId = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(PRICE));
        val && val->is_string()) {
      response.price = Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(ORIG_QTY));
        val && val->is_string()) {
      response.origQty =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(EXECUTED_QTY));
        val && val->is_string()) {
      response.executedQty =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(CUM_QUOTE));
        val && val->is_string()) {
      response.cumQuote =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(AVG_PRICE));
        val && val->is_string()) {
      response.avgPrice =
          Fixed::str_to_fixed(val->get_ref<const std::string &>());
    }

    if (auto val = json.get_value(std::string(STATUS));
        val && val->is_string()) {
      response.status = val->get<std::string>();
    }

    if (auto val = json.get_value(std::string(UPDATE_TIME));
        val && val->is_number_unsigned()) {
      response.updateTime = val->get<uint64_t>();
    }

    if (auto val = json.get_value(std::string(WORKING_TIME));
        val && val->is_number_unsigned()) {
      response.workingTime = val->get<uint64_t>();
    }

    // Parse enums
    if (auto val = json.get_value(std::string(TYPE)); val && val->is_string()) {
      response.type = str_to_type(ORDER_TYPE_STR, val->get<std::string>());
    }

    if (auto val = json.get_value(std::string(SIDE)); val && val->is_string()) {
      response.side = str_to_type(ORDER_SIDE_STR, val->get<std::string>());
    }

    if (auto val = json.get_value(std::string(POSITION_SIDE));
        val && val->is_string()) {
      response.positionSide =
          str_to_type(POSITION_SIDE_STR, val->get<std::string>());
    }

    if (auto val = json.get_value(std::string(TIME_IN_FORCE));
        val && val->is_string()) {
      response.timeInForce =
          str_to_type(TIME_IN_FORCE_STR, val->get<std::string>());
    }

    return response;
  } catch (const nlohmann::json::exception &e) {
    ErrorParse error;
    error.parse_error = std::string("OrderStatusParser error: ") + e.what();
    return error;
  }
}

// ============================================================================
// Account Position Parser
// ============================================================================
ParsedTradingStream AccountPositionParser::parse(const ResultMessage &msg) {
  try {
    AccountPositionResponse response;

    const JSONQuery &json = msg.result_msg;

    if (json.is_empty()) {
      return ErrorParse{"JSON is empty"};
    }

    // Check if result is array
    if (!json.is_array()) {
      ErrorParse error;
      error.parse_error = "AccountPositionParser: expected array";
      return error;
    }

    auto positionsArray = json.get_array();
    if (!positionsArray) {
      ErrorParse error;
      error.parse_error = "AccountPositionParser: failed to get array";
      return error;
    }

    for (const auto &positionJson : *positionsArray) {
      JSONQuery positionQuery(positionJson);

      AccountPositionResponse::Position position;

      if (auto val = positionQuery.get_value(std::string(SYMBOL));
          val && val->is_string()) {
        position.symbol = val->get<std::string>();
      }

      if (auto val = positionQuery.get_value(std::string(POSITION_AMT));
          val && val->is_string()) {
        position.positionAmt =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(ENTRY_PRICE));
          val && val->is_string()) {
        position.entryPrice =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(BREAK_EVEN_PRICE));
          val && val->is_string()) {
        position.breakEvenPrice =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(MARK_PRICE));
          val && val->is_string()) {
        position.markPrice =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(UN_REALIZED_PROFIT));
          val && val->is_string()) {
        position.unRealizedProfit =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(LIQUIDATION_PRICE));
          val && val->is_string()) {
        position.liquidationPrice =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(LEVERAGE));
          val && val->is_number_unsigned()) {
        position.leverage = val->get<uint32_t>();
      }

      if (auto val = positionQuery.get_value(std::string(MAX_NOTIONAL_VALUE));
          val && val->is_string()) {
        position.maxNotionalValue =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(ISOLATED));
          val && val->is_boolean()) {
        position.isolated = val->get<bool>();
      }

      if (auto val = positionQuery.get_value(std::string(NOTIONAL));
          val && val->is_string()) {
        position.notional =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(ISOLATED_WALLET));
          val && val->is_string()) {
        position.isolatedWallet =
            Fixed::str_to_fixed(val->get_ref<const std::string &>());
      }

      if (auto val = positionQuery.get_value(std::string(UPDATE_TIME));
          val && val->is_number_unsigned()) {
        position.updateTime = val->get<uint64_t>();
      }

      // Parse enums
      if (auto val = positionQuery.get_value(std::string(MARGIN_TYPE));
          val && val->is_string()) {
        position.marginType =
            str_to_type(MARGIN_TYPE_STR, val->get<std::string>());
      }

      if (auto val = positionQuery.get_value(std::string(POSITION_SIDE));
          val && val->is_string()) {
        position.positionSide =
            str_to_type(POSITION_SIDE_STR, val->get<std::string>());
      }

      response.positions.push_back(position);
    }

    return response;
  } catch (const nlohmann::json::exception &e) {
    ErrorParse error;
    error.parse_error = std::string("AccountPositionParser error: ") + e.what();
    return error;
  }
}
