#include "market_data_parser.hpp"

#include "core/utils/helper_utils.hpp"
#include "core/utils/json.hpp"

using namespace Market;

/* static */ ParsedMarketData
MarketDataParser::parse(const StreamMessage &msg) {
  if (msg.stream.empty() || msg.data.is_empty())
    return ErrorParse{"StreamMessage stream or data is empty"};

  switch (_detect_msg_type(msg)) {
  case MARKET_DATA_TYPE::TRADE:
    return TradeDataParser::parse(msg);
  case MARKET_DATA_TYPE::AGG_TRADE:
    return AggTradeDataParser::parse(msg);
  case MARKET_DATA_TYPE::MARK_PRICE:
    return MarkPriceDataParser::parse(msg);
  // Fallback since this diff and part have same parser
  case MARKET_DATA_TYPE::DIFF_DEPTH:
  case MARKET_DATA_TYPE::PART_DEPTH:
    return DepthDataParser::parse(msg);
  case MARKET_DATA_TYPE::BOOK_TICKER:
    return BookTickerDataParser::parse(msg);
  default:
    return ErrorParse{"StreamMessage type is invalid"};
  }
}

/* static */ MARKET_DATA_TYPE
MarketDataParser::_detect_msg_type(const StreamMessage &msg) {
  for (const auto &elem : MARKET_DATA_TYPE_STR) {
    if (msg.stream.find(std::string(elem.str)) != std::string::npos) {
      return elem.type;
    }
  }

  return MARKET_DATA_TYPE::UNKNOWN;
}

/* static */ ParsedMarketData TradeDataParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    TradeData data;

    if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"eventTime is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(TRADE_TIME));
      val && val->is_number_unsigned()) {
    data.tradeTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"tradeTime is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(SYMBOL));
      val && val->is_string()) {
    data.symbol = val->get<std::string>();
  } else {
    return ErrorParse{"symbol is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(TRADE_ID));
      val && val->is_number_unsigned()) {
    data.tradeId = val->get<uint64_t>();
  } else {
    return ErrorParse{"tradeId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(PRICE));
      val && val->is_string()) {
    const std::string &priceStr = val->get_ref<const std::string &>();
    data.price = Fixed::str_to_fixed(priceStr);
  } else {
    return ErrorParse{"price is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(QUANTITY));
      val && val->is_string()) {
    const std::string &quantityStr = val->get_ref<const std::string &>();
    data.quantity = Fixed::str_to_fixed(quantityStr);
  } else {
    return ErrorParse{"quantity is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(ORDER_T));
      val && val->is_string()) {
    const std::string &orderTypeStr = val->get_ref<const std::string &>();
    ORDER_TYPE type = str_to_type(ORDER_TYPE_STR, orderTypeStr);
    data.orderType = type;
  } else {
    return ErrorParse{"orderType is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(MARKET_MAKER));
      val && val->is_boolean()) {
    data.isMarketMaker = val->get<bool>();
  } else {
    return ErrorParse{"isMarketMaker is not parsed"};
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in TradeDataParser: " + std::string(e.what())};
  }
}

/* static */ ParsedMarketData
AggTradeDataParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    AggTradeData data;

    if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"eventTime is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(AGG_TRADE_ID));
      val && val->is_number_unsigned()) {
    data.aggTradeId = val->get<uint64_t>();
  } else {
    return ErrorParse{"aggTradeId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(SYMBOL));
      val && val->is_string()) {
    data.symbol = val->get<std::string>();
  } else {
    return ErrorParse{"symbol is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(PRICE));
      val && val->is_string()) {
    const std::string &priceStr = val->get_ref<const std::string &>();
    data.price = Fixed::str_to_fixed(priceStr);
  } else {
    return ErrorParse{"price is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(QUANTITY));
      val && val->is_string()) {
    const std::string &quantityStr = val->get_ref<const std::string &>();
    data.quantity = Fixed::str_to_fixed(quantityStr);
  } else {
    return ErrorParse{"quantity is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(NORMAL_QUANTITY));
      val && val->is_string()) {
    const std::string &normQtyStr = val->get_ref<const std::string &>();
    data.normalQuantity = Fixed::str_to_fixed(normQtyStr);
  } else {
    return ErrorParse{"normalQuantity is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(FIRST_ORDER_ID));
      val && val->is_number_unsigned()) {
    data.firstTradeId = val->get<uint64_t>();
  } else {
    return ErrorParse{"firstTradeId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(LAST_ORDER_ID));
      val && val->is_number_unsigned()) {
    data.lastTradeId = val->get<uint64_t>();
  } else {
    return ErrorParse{"lastTradeId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(TRADE_TIME));
      val && val->is_number_unsigned()) {
    data.tradeTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"tradeTime is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(MARKET_MAKER));
      val && val->is_boolean()) {
    data.isMarketMaker = val->get<bool>();
  } else {
    return ErrorParse{"isMarketMaker is not parsed"};
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in AggTradeDataParser: " + std::string(e.what())};
  }
}

/* static */ ParsedMarketData
MarkPriceDataParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    MarkPriceData data;

    if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"eventTime is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(SYMBOL));
      val && val->is_string()) {
    data.symbol = val->get<std::string>();
  } else {
    return ErrorParse{"symbol is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(MARK_PRICE));
      val && val->is_string()) {
    const std::string &str = val->get_ref<const std::string &>();
    data.markPrice = Fixed::str_to_fixed(str);
  } else {
    return ErrorParse{"markPrice is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(SETTLE_PRICE));
      val && val->is_string()) {
    const std::string &str = val->get_ref<const std::string &>();
    data.settlePrice = Fixed::str_to_fixed(str);
  } else {
    return ErrorParse{"settlePrice is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(INDEX_PRICE));
      val && val->is_string()) {
    const std::string &str = val->get_ref<const std::string &>();
    data.indexPrice = Fixed::str_to_fixed(str);
  } else {
    return ErrorParse{"indexPrice is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(FUNDING_RATE));
      val && val->is_string()) {
    const std::string &str = val->get_ref<const std::string &>();
    data.fundingRate = Fixed::str_to_fixed(str);
  } else {
    return ErrorParse{"fundingRate is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(NEXT_FUNDING_RATE));
      val && val->is_number_unsigned()) {
    data.nextFundingTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"nextFundingTime is not parsed"};
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in MarkPriceDataParser: " + std::string(e.what())};
  }
}

/* static */ bool DepthDataParser::parsePriceQty(const nlohmann::json &entry,
                                                 Fixed &price, Fixed &qty) {
  if (!entry.is_array() || entry.size() != 2) {
    return false;
  }

  if (!entry[0].is_string() || !entry[1].is_string()) {
    return false;
  }

  price = Fixed::str_to_fixed(entry[0].get<std::string>());
  qty = Fixed::str_to_fixed(entry[1].get<std::string>());

  return true;
}

/* static */ ParsedMarketData DepthDataParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    DepthData data;

    if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"eventTime is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(TRANSACTION_TIME));
      val && val->is_number_unsigned()) {
    data.transactionTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"transactionTime is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(SYMBOL));
      val && val->is_string()) {
    data.symbol = val->get<std::string>();
  } else {
    return ErrorParse{"symbol is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(FIRST_UPDT_ID));
      val && val->is_number_unsigned()) {
    data.firstUpdateId = val->get<uint64_t>();
  } else {
    return ErrorParse{"firstUpdateId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(FINAL_UPDT_ID));
      val && val->is_number_unsigned()) {
    data.finalUpdateId = val->get<uint64_t>();
  } else {
    return ErrorParse{"finalUpdateId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(FINAL_UPDT_ID_LAST_STREAM));
      val && val->is_number_unsigned()) {
    data.finalUpdtIdLastStream = val->get<uint64_t>();
  } else {
    return ErrorParse{"finalUpdtIdLastStream is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(BIDS));
      val && val->is_array()) {
    for (const auto &entry : val->get<nlohmann::json>()) {
      Fixed price, qty;
      if (parsePriceQty(entry, price, qty)) {
        data.bids.emplace_back(price, qty);
      } else {
        return ErrorParse{"array of bids is not parsed"};
      }
    }
  } else {
    return ErrorParse{"bids are not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(ASKS));
      val && val->is_array()) {
    for (const auto &entry : val->get<nlohmann::json>()) {
      Fixed price, qty;
      if (parsePriceQty(entry, price, qty)) {
        data.asks.emplace_back(price, qty);
      } else {
        return ErrorParse{"array of asks is not parsed"};
      }
    }
  } else {
    return ErrorParse{"asks are not parsed"};
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in DepthDataParser: " + std::string(e.what())};
  }
}

/* static */ ParsedMarketData
BookTickerDataParser::parse(const StreamMessage &msg) {
  try {
    JSONQuery jsonData = msg.data;
    BookTickerData data;

    if (auto val = jsonData.get_value(std::string(ORDER_BOOK_ID));
      val && val->is_number_unsigned()) {
    data.orderBookId = val->get<uint64_t>();
  } else {
    return ErrorParse{"orderBookId is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(SYMBOL));
      val && val->is_string()) {
    data.symbol = val->get<std::string>();
  } else {
    return ErrorParse{"symbol is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(BID_PRICE));
      val && val->is_string()) {
    const std::string &str = val->get_ref<const std::string &>();
    data.bestBidPrice = Fixed::str_to_fixed(str);
  } else {
    return ErrorParse{"bestBidPrice is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(BID_QTY));
      val && val->is_string()) {
    const std::string &str = val->get_ref<const std::string &>();
    data.bestBidQty = Fixed::str_to_fixed(str);
  } else {
    return ErrorParse{"bestBidQty is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(ASK_PRICE));
      val && val->is_string()) {
    const std::string &str = val->get_ref<const std::string &>();
    data.bestAskPrice = Fixed::str_to_fixed(str);
  } else {
    return ErrorParse{"bestAskPrice is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(ASK_QTY));
      val && val->is_string()) {
    const std::string &str = val->get_ref<const std::string &>();
    data.bestAskQty = Fixed::str_to_fixed(str);
  } else {
    return ErrorParse{"bestAskQty is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(TRANSACTION_TIME));
      val && val->is_number_unsigned()) {
    data.transactionTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"transactionTime is not parsed"};
  }

  if (auto val = jsonData.get_value(std::string(EVENT_TIME));
      val && val->is_number_unsigned()) {
    data.eventTime = val->get<uint64_t>();
  } else {
    return ErrorParse{"eventTime is not parsed"};
  }

  return data;
  } catch (const nlohmann::json::exception &e) {
    return ErrorParse{"JSON exception in BookTickerDataParser: " + std::string(e.what())};
  }
}
