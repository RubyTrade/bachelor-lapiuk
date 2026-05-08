#ifndef USER_DATA_STREAM_PARSER_HPP
#define USER_DATA_STREAM_PARSER_HPP

#include "core/controllers/user_data_stream_utils.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <variant>

namespace UserData {

struct TradeLiteEvent {
  USER_DATA_EVENT_TYPE eventType = USER_DATA_EVENT_TYPE::TRADE_LITE;

  uint64_t orderId = 0;
  uint64_t tradeId = 0;

  std::string symbol{};
  std::string clientOrderId{};

  ORDER_SIDE side{};

  Fixed price{};
  Fixed qty{};
  Fixed lastPrice{};
  Fixed lastQty{};

  bool isMaker = false;

  uint64_t eventTime = 0;
  uint64_t tradeTime = 0;
};

struct OrderTradeUpdateEvent {
  USER_DATA_EVENT_TYPE eventType = USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE;

  uint64_t orderId = 0;        // i
  std::string symbol{};        // s
  std::string clientOrderId{}; // c

  EXECUTION_TYPE executionType{}; // x
  ORDER_STATUS status{};          // X

  Fixed price{};     // p
  Fixed avgPrice{};  // ap
  Fixed stopPrice{}; // sp
  Fixed lastPrice{}; // L

  Fixed origQty{};         // q
  Fixed executedQty{};     // z
  Fixed lastExecutedQty{}; // l
  Fixed cumQuote{};        // Z

  // fees
  Fixed commission{};            // n
  std::string commissionAsset{}; // N

  // pnl
  Fixed realizedPnL{}; // rp

  // misc trade
  uint64_t tradeId = 0; // t
  bool isMaker = false; // m

  Fixed bidNotional{}; // b
  Fixed askNotional{}; // a

  TIME_IN_FORCE timeInForce{}; // f
  ORDER_TYPE orderType{};      // o
  ORDER_TYPE originalType{};   // ot

  ORDER_SIDE side{};            // S
  POSITION_SIDE positionSide{}; // ps

  bool reduceOnly = false;    // R
  bool closePosition = false; // cp
  bool priceProtect = false;  // pP

  WORKING_TYPE workingType{};                           // wt
  std::string priceMatch{};                             // pm
  SELF_TRADE_PREVENTION_MODE selfTradePreventionMode{}; // V

  uint64_t goodTillDate = 0; // gtd

  // time
  uint64_t eventTime = 0;       // E
  uint64_t transactionTime = 0; // T (top)
  uint64_t tradeTime = 0;       // T (inside order)
};

struct AccountUpdateEvent {
  USER_DATA_EVENT_TYPE eventType = USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE;

  struct Balance {
    std::string asset{};
    Fixed walletBalance{};
    Fixed crossWalletBalance{};
    Fixed balanceChange{};
  };

  struct Position {
    std::string symbol{};
    Fixed positionAmt{};
    Fixed entryPrice{};
    Fixed realizedPnL{};
    Fixed unrealizedPnL{};
    Fixed isolatedWallet{};
    POSITION_SIDE positionSide{};
    std::string marginAsset{};
    Fixed breakEvenPrice{};
  };

  std::vector<Balance> balances;
  std::vector<Position> positions;

  std::string reason{};

  uint64_t eventTime = 0;
  uint64_t transactionTime = 0;
};

struct MarginCallEvent {
  USER_DATA_EVENT_TYPE eventType = USER_DATA_EVENT_TYPE::MARGIN_CALL;

  struct Position {
    std::string symbol{};
    POSITION_SIDE positionSide{};
    Fixed positionAmt{};
    std::string marginType{};
    Fixed isolatedWallet{};
    Fixed markPrice{};
    Fixed unrealizedPnL{};
    Fixed maintenanceMargin{};
  };

  Fixed crossWalletBalance{};
  std::vector<Position> positions;

  uint64_t eventTime = 0;
};

struct AccountConfigUpdateEvent {
  USER_DATA_EVENT_TYPE eventType = USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE;

  struct LeverageUpdate {
    std::string symbol{};
    uint32_t leverage = 0;
  };

  struct MultiAssetModeUpdate {
    bool multiAssetsMode = false;
  };

  std::optional<LeverageUpdate> leverageUpdate;
  std::optional<MultiAssetModeUpdate> multiAssetModeUpdate;

  uint64_t eventTime = 0;
  uint64_t transactionTime = 0;
};

using ParsedUserData =
    std::variant<OrderTradeUpdateEvent, TradeLiteEvent, AccountUpdateEvent,
                 MarginCallEvent, AccountConfigUpdateEvent, ErrorParse>;

class OrderTradeUpdateParser {
public:
  static ParsedUserData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view EVENT_TYPE = "e";
  static constexpr std::string_view ORDER = "o";

  // --- IDS / BASIC ---
  static constexpr std::string_view ORDER_ID = "i";
  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view CLIENT_ORDER_ID = "c";
  static constexpr std::string_view EXECUTION_TYPE = "x";
  static constexpr std::string_view STATUS = "X";

  // --- PRICE ---
  static constexpr std::string_view PRICE = "p";
  static constexpr std::string_view AVG_PRICE = "ap";
  static constexpr std::string_view STOP_PRICE = "sp";
  static constexpr std::string_view LAST_PRICE = "L";

  // --- QTY ---
  static constexpr std::string_view ORIG_QTY = "q";
  static constexpr std::string_view EXECUTED_QTY = "z";
  static constexpr std::string_view LAST_EXECUTED_QTY = "l";
  static constexpr std::string_view CUM_QTY = "Z";

  // --- FEES ---
  static constexpr std::string_view COMMISSION = "n";
  static constexpr std::string_view COMMISSION_ASSET = "N";

  // --- PNL ---
  static constexpr std::string_view REALIZED_PNL = "rp";

  // --- TRADE INFO ---
  static constexpr std::string_view TRADE_ID = "t";
  static constexpr std::string_view IS_MAKER = "m";
  static constexpr std::string_view BID_NOTIONAL = "b";
  static constexpr std::string_view ASK_NOTIONAL = "a";

  // --- TYPES ---
  static constexpr std::string_view TIME_IN_FORCE = "f";
  static constexpr std::string_view TYPE = "o";
  static constexpr std::string_view ORIG_TYPE = "ot";

  static constexpr std::string_view SIDE = "S";
  static constexpr std::string_view POSITION_SIDE = "ps";

  // --- FLAGS ---
  static constexpr std::string_view REDUCE_ONLY = "R";
  static constexpr std::string_view CLOSE_POSITION = "cp";
  static constexpr std::string_view PRICE_PROTECT = "pP";

  // --- EXTRA ---
  static constexpr std::string_view WORKING_TYPE = "wt";
  static constexpr std::string_view PRICE_MATCH = "pm";
  static constexpr std::string_view STP_MODE = "V";

  static constexpr std::string_view GOOD_TILL_DATE = "gtd";

  // --- TIME ---
  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view TRANSACTION_TIME = "T"; // top-level
  static constexpr std::string_view TRADE_TIME = "T";       // inside order
};

class TradeLiteParser {
public:
  static ParsedUserData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view CLIENT_ORDER_ID = "c";
  static constexpr std::string_view SIDE = "S";

  static constexpr std::string_view PRICE = "p";
  static constexpr std::string_view QTY = "q";
  static constexpr std::string_view LAST_PRICE = "L";
  static constexpr std::string_view LAST_QTY = "l";

  static constexpr std::string_view MAKER = "m";

  static constexpr std::string_view ORDER_ID = "i";
  static constexpr std::string_view TRADE_ID = "t";

  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view TRADE_TIME = "T";
};

class AccountUpdateParser {
public:
  static ParsedUserData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view ACCOUNT = "a";

  static constexpr std::string_view BALANCES = "B";
  static constexpr std::string_view POSITIONS = "P";

  static constexpr std::string_view ASSET = "a";
  static constexpr std::string_view WALLET_BALANCE = "wb";
  static constexpr std::string_view CROSS_WALLET_BALANCE = "cw";
  static constexpr std::string_view BALANCE_CHANGE = "bc";

  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view POSITION_AMT = "pa";
  static constexpr std::string_view ENTRY_PRICE = "ep";
  static constexpr std::string_view REALIZED_PNL = "cr";
  static constexpr std::string_view UNREALIZED_PNL = "up";
  static constexpr std::string_view ISOLATED_WALLET = "iw";
  static constexpr std::string_view POSITION_SIDE = "ps";
  static constexpr std::string_view MARGIN_ASSET = "ma";
  static constexpr std::string_view BREAK_EVEN_PRICE = "bep";

  static constexpr std::string_view REASON = "m";

  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view TRANSACTION_TIME = "T";
};

class MarginCallParser {
public:
  static ParsedUserData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view CROSS_WALLET = "cw";
  static constexpr std::string_view POSITIONS = "p";

  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view POSITION_SIDE = "ps";
  static constexpr std::string_view POSITION_AMT = "pa";
  static constexpr std::string_view MARGIN_TYPE = "mt";
  static constexpr std::string_view ISOLATED_WALLET = "iw";
  static constexpr std::string_view MARK_PRICE = "mp";
  static constexpr std::string_view UNREALIZED_PNL = "up";
  static constexpr std::string_view MAINT_MARGIN = "mm";

  static constexpr std::string_view EVENT_TIME = "E";
};

class AccountConfigUpdateParser {
public:
  static ParsedUserData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view LEVERAGE_OBJ = "ac";
  static constexpr std::string_view MULTI_ASSET_OBJ = "ai";

  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view LEVERAGE = "l";

  static constexpr std::string_view MULTI_ASSETS_MODE = "j";

  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view TRANSACTION_TIME = "T";
};

class AlgoUpdateParser {
public:
  static ParsedUserData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view ORDER = "o";

  static constexpr std::string_view CLIENT_ALGO_ID = "caid";
  static constexpr std::string_view ALGO_ID = "aid";
  static constexpr std::string_view MATCH_ORDER_ID = "ai";
  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view SIDE = "S";
  static constexpr std::string_view POSITION_SIDE = "ps";
  static constexpr std::string_view ORDER_TYPE = "o";
  static constexpr std::string_view TIME_IN_FORCE = "f";
  static constexpr std::string_view ALGO_STATUS = "X";

  static constexpr std::string_view ORIG_QTY = "q";
  static constexpr std::string_view EXECUTED_QTY = "aq";
  static constexpr std::string_view AVG_PRICE = "ap";
  static constexpr std::string_view PRICE = "p";
  static constexpr std::string_view TRIGGER_PRICE = "tp";

  static constexpr std::string_view REDUCE_ONLY = "R";
  static constexpr std::string_view CLOSE_POSITION = "cp";
  static constexpr std::string_view PRICE_PROTECT = "pP";

  static constexpr std::string_view WORKING_TYPE = "wt";
  static constexpr std::string_view PRICE_MATCH = "pm";
  static constexpr std::string_view STP_MODE = "V";

  static constexpr std::string_view GOOD_TILL_DATE = "gtd";
  static constexpr std::string_view TRIGGER_TIME = "tt";

  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view TRANSACTION_TIME = "T";
};

// Main Parser
class UserDataStreamParser {
public:
  static ParsedUserData parse(const StreamMessage &msg);
};

} // namespace UserData

#endif // USER_DATA_STREAM_PARSER_HPP

/*
 *
{"e":"TRADE_LITE","E":1771856732584,"T":1771856732584,"s":"SOLUSDT","q":"0.10","p":"0.0000","m":false,"c":"SOLUSDT_BOTH_1771856732181_1",
"S":"BUY","L":"80.1900","l":"0.10","t":3181138052,"i":199033476993}

{"e":"ORDER_TRADE_UPDATE","T":1771856732584,"E":1771856732584,"o":{"s":"SOLUSDT","c":"SOLUSDT_BOTH_1771856732181_1","S":"BUY","o":"MARKET
","f":"GTC","q":"0.1","p":"0","ap":"0","sp":"0","x":"NEW","X":"NEW","i":199033476993,"l":"0","z":"0","L":"0","n":"0","N":"USDT","T":1771856732584,"
t":0,"b":"0","a":"0","m":false,"R":false,"wt":"CONTRACT_PRICE","ot":"MARKET","ps":"BOTH","cp":false,"rp":"0","pP":false,"si":0,"ss":0,"V":"EXPIRE_M
AKER","pm":"NONE","gtd":0,"er":"0"}}

{"e":"ACCOUNT_UPDATE","T":1771856732584,"E":1771856732584,"a":{"B":[{"a":"USDT","wb":"1.73744723","cw":"1.73744723","bc":"0"}],"P":[{"s":
"SOLUSDT","pa":"0.1","ep":"80.19","cr":"26.83159999","up":"0","mt":"cross","iw":"0","ps":"BOTH","ma":"USDT","bep":"80.230095"}],"m":"ORDER"}}

*/
