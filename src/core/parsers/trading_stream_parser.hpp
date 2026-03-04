#ifndef TRADING_STREAM_PARSER_HPP
#define TRADING_STREAM_PARSER_HPP

#include "core/controllers/trading_stream_utils.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace Trading {

// ============================================================================
// Account Status Response
// ============================================================================
struct AccountStatusResponse {
  TRADE_STREAM_METHOD apiType = TRADE_STREAM_METHOD::ACCOUNT_STATUS;

  bool canTrade = false;
  bool canDeposit = false;
  bool canWithdraw = false;
  uint64_t updateTime = 0;
};

// ============================================================================
// Account Balance Response
// ============================================================================
struct AccountBalanceResponse {
  TRADE_STREAM_METHOD apiType = TRADE_STREAM_METHOD::ACCOUNT_BALANCE;

  struct Balance {
    std::string asset{};
    Fixed balance{};
    Fixed crossWalletBalance{};
    Fixed crossUnPnl{};
    Fixed availableBalance{};
    Fixed maxWithdrawAmount{};
    bool marginAvailable = false;
    uint64_t updateTime = 0;
  };

  std::vector<Balance> balances;
};

// ============================================================================
// Order Place Response
// ============================================================================
struct OrderPlaceResponse {
  TRADE_STREAM_METHOD apiType = TRADE_STREAM_METHOD::ORDER_PLACE;

  std::string symbol{};
  int64_t orderId = 0;
  std::string clientOrderId{};
  Fixed price{};
  Fixed origQty{};
  Fixed executedQty{};
  Fixed cumQuote{};
  ORDER_TYPE type{};
  ORDER_SIDE side{};
  POSITION_SIDE positionSide{};
  TIME_IN_FORCE timeInForce{};
  std::string status{};
  uint64_t updateTime = 0;
  uint64_t workingTime = 0;
};

// ============================================================================
// Order Modify Response
// ============================================================================
struct OrderModifyResponse {
  TRADE_STREAM_METHOD apiType = TRADE_STREAM_METHOD::ORDER_MODIFY;

  std::string symbol{};
  int64_t orderId = 0;
  std::string clientOrderId{};
  Fixed price{};
  Fixed origQty{};
  std::string status{};
  uint64_t updateTime = 0;
};

// ============================================================================
// Order Cancel Response
// ============================================================================
struct OrderCancelResponse {
  TRADE_STREAM_METHOD apiType = TRADE_STREAM_METHOD::ORDER_CANCEL;

  std::string symbol{};
  int64_t orderId = 0;
  std::string clientOrderId{};
  std::string status{};
  uint64_t updateTime = 0;
};

// ============================================================================
// Order Status Response
// ============================================================================
struct OrderStatusResponse {
  TRADE_STREAM_METHOD apiType = TRADE_STREAM_METHOD::ORDER_STATUS;

  std::string symbol{};
  int64_t orderId = 0;
  std::string clientOrderId{};
  Fixed price{};
  Fixed origQty{};
  Fixed executedQty{};
  Fixed cumQuote{};
  ORDER_TYPE type{};
  ORDER_SIDE side{};
  POSITION_SIDE positionSide{};
  TIME_IN_FORCE timeInForce{};
  std::string status{};
  Fixed avgPrice{};
  uint64_t updateTime = 0;
  uint64_t workingTime = 0;
};

// ============================================================================
// Account Position Response
// ============================================================================
struct AccountPositionResponse {
  TRADE_STREAM_METHOD apiType = TRADE_STREAM_METHOD::ACCOUNT_POSITION;

  struct Position {
    std::string symbol{};
    Fixed positionAmt{};
    Fixed entryPrice{};
    Fixed breakEvenPrice{};
    Fixed markPrice{};
    Fixed unRealizedProfit{};
    Fixed liquidationPrice{};
    uint32_t leverage = 0;
    Fixed maxNotionalValue{};
    MARGIN_TYPE marginType{};
    bool isolated = false;
    POSITION_SIDE positionSide{};
    Fixed notional{};
    Fixed isolatedWallet{};
    uint64_t updateTime = 0;
  };

  std::vector<Position> positions;
};

struct TradingResultStream {
  TRADE_STREAM_METHOD type;
  ResultMessage result;

  TradingResultStream(TRADE_STREAM_METHOD t, ResultMessage r)
      : type(t), result(r) {}
};

using ParsedTradingStream =
    std::variant<AccountStatusResponse, AccountBalanceResponse,
                 OrderPlaceResponse, OrderModifyResponse, OrderCancelResponse,
                 OrderStatusResponse, AccountPositionResponse, ErrorParse>;

class TradingStreamParser {
public:
  static ParsedTradingStream parse(const TradingResultStream &msg);
};

class AccountStatusParser {
public:
  static ParsedTradingStream parse(const ResultMessage &msg);

private:
  static constexpr std::string_view CAN_TRADE = "canTrade";
  static constexpr std::string_view CAN_DEPOSIT = "canDeposit";
  static constexpr std::string_view CAN_WITHDRAW = "canWithdraw";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
};

class AccountBalanceParser {
public:
  static ParsedTradingStream parse(const ResultMessage &msg);

private:
  static constexpr std::string_view ASSET = "asset";
  static constexpr std::string_view BALANCE = "balance";
  static constexpr std::string_view CROSS_WALLET_BALANCE = "crossWalletBalance";
  static constexpr std::string_view CROSS_UN_PNL = "crossUnPnl";
  static constexpr std::string_view AVAILABLE_BALANCE = "availableBalance";
  static constexpr std::string_view MAX_WITHDRAW_AMOUNT = "maxWithdrawAmount";
  static constexpr std::string_view MARGIN_AVAILABLE = "marginAvailable";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
};

class OrderPlaceParser {
public:
  static ParsedTradingStream parse(const ResultMessage &msg);

private:
  static constexpr std::string_view SYMBOL = "symbol";
  static constexpr std::string_view ORDER_ID = "orderId";
  static constexpr std::string_view CLIENT_ORDER_ID = "clientOrderId";
  static constexpr std::string_view PRICE = "price";
  static constexpr std::string_view ORIG_QTY = "origQty";
  static constexpr std::string_view EXECUTED_QTY = "executedQty";
  static constexpr std::string_view CUM_QUOTE = "cumQuote";
  static constexpr std::string_view TYPE = "type";
  static constexpr std::string_view SIDE = "side";
  static constexpr std::string_view POSITION_SIDE = "positionSide";
  static constexpr std::string_view TIME_IN_FORCE = "timeInForce";
  static constexpr std::string_view STATUS = "status";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
  static constexpr std::string_view WORKING_TIME = "workingTime";
};

class OrderModifyParser {
public:
  static ParsedTradingStream parse(const ResultMessage &msg);

private:
  static constexpr std::string_view SYMBOL = "symbol";
  static constexpr std::string_view ORDER_ID = "orderId";
  static constexpr std::string_view CLIENT_ORDER_ID = "clientOrderId";
  static constexpr std::string_view PRICE = "price";
  static constexpr std::string_view ORIG_QTY = "origQty";
  static constexpr std::string_view STATUS = "status";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
};

class OrderCancelParser {
public:
  static ParsedTradingStream parse(const ResultMessage &msg);

private:
  static constexpr std::string_view SYMBOL = "symbol";
  static constexpr std::string_view ORDER_ID = "orderId";
  static constexpr std::string_view CLIENT_ORDER_ID = "clientOrderId";
  static constexpr std::string_view STATUS = "status";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
};

class OrderStatusParser {
public:
  static ParsedTradingStream parse(const ResultMessage &msg);

private:
  static constexpr std::string_view SYMBOL = "symbol";
  static constexpr std::string_view ORDER_ID = "orderId";
  static constexpr std::string_view CLIENT_ORDER_ID = "clientOrderId";
  static constexpr std::string_view PRICE = "price";
  static constexpr std::string_view ORIG_QTY = "origQty";
  static constexpr std::string_view EXECUTED_QTY = "executedQty";
  static constexpr std::string_view CUM_QUOTE = "cumQuote";
  static constexpr std::string_view TYPE = "type";
  static constexpr std::string_view SIDE = "side";
  static constexpr std::string_view POSITION_SIDE = "positionSide";
  static constexpr std::string_view TIME_IN_FORCE = "timeInForce";
  static constexpr std::string_view STATUS = "status";
  static constexpr std::string_view AVG_PRICE = "avgPrice";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
  static constexpr std::string_view WORKING_TIME = "workingTime";
};

class AccountPositionParser {
public:
  static ParsedTradingStream parse(const ResultMessage &msg);

private:
  static constexpr std::string_view SYMBOL = "symbol";
  static constexpr std::string_view POSITION_AMT = "positionAmt";
  static constexpr std::string_view ENTRY_PRICE = "entryPrice";
  static constexpr std::string_view BREAK_EVEN_PRICE = "breakEvenPrice";
  static constexpr std::string_view MARK_PRICE = "markPrice";
  static constexpr std::string_view UN_REALIZED_PROFIT = "unRealizedProfit";
  static constexpr std::string_view LIQUIDATION_PRICE = "liquidationPrice";
  static constexpr std::string_view LEVERAGE = "leverage";
  static constexpr std::string_view MAX_NOTIONAL_VALUE = "maxNotionalValue";
  static constexpr std::string_view MARGIN_TYPE = "marginType";
  static constexpr std::string_view ISOLATED = "isolated";
  static constexpr std::string_view POSITION_SIDE = "positionSide";
  static constexpr std::string_view NOTIONAL = "notional";
  static constexpr std::string_view ISOLATED_WALLET = "isolatedWallet";
  static constexpr std::string_view UPDATE_TIME = "updateTime";
};

} // namespace Trading

#endif // TRADING_STREAM_PARSER_HPP
