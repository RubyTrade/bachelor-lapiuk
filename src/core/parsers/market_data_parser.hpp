#ifndef MARKET_DATA_PARSER_HPP
#define MARKET_DATA_PARSER_HPP

#include "core/controllers/market_data_utils.hpp"
#include "core/parsers/common_parser_utils.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/json.hpp"

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <utility>
#include <variant>

namespace Market {

struct TradeData {
  MARKET_DATA_TYPE eventType = MARKET_DATA_TYPE::TRADE; // e

  uint64_t eventTime;   // E
  uint64_t tradeTime;   // T
  std::string symbol;   // s
  uint64_t tradeId;     // t
  Fixed price;          // p
  Fixed quantity;       // q
  ORDER_TYPE orderType; // X
  bool isMarketMaker;   // m
};

struct AggTradeData {
  MARKET_DATA_TYPE eventType = MARKET_DATA_TYPE::AGG_TRADE; // e

  uint64_t eventTime;    // E
  uint64_t aggTradeId;   // a
  std::string symbol;    // s
  Fixed price;           // p
  Fixed quantity;        // q
  Fixed normalQuantity;  // nq
  uint64_t firstTradeId; // f
  uint64_t lastTradeId;  // l
  uint64_t tradeTime;    // T
  bool isMarketMaker;    // m
};

struct MarkPriceData {
  MARKET_DATA_TYPE eventType = MARKET_DATA_TYPE::MARK_PRICE; // e

  uint64_t eventTime;       // E
  std::string symbol;       // s
  Fixed markPrice;          // p
  Fixed settlePrice;        // P
  Fixed indexPrice;         // i
  Fixed fundingRate;        // r
  uint64_t nextFundingTime; // T
};

struct DepthData {
  MARKET_DATA_TYPE eventType = MARKET_DATA_TYPE::DIFF_DEPTH; // e

  uint64_t eventTime;                        // E
  uint64_t transactionTime;                  // T
  std::string symbol;                        // s
  uint64_t firstUpdateId;                    // U
  uint64_t finalUpdateId;                    // u
  uint64_t finalUpdtIdLastStream;            // pu
  std::vector<std::pair<Fixed, Fixed>> bids; // b
  std::vector<std::pair<Fixed, Fixed>> asks; // a
  // pair: first - price, second - quantity
};

struct BookTickerData {
  MARKET_DATA_TYPE eventType = MARKET_DATA_TYPE::BOOK_TICKER; // e

  uint64_t orderBookId;     // u
  std::string symbol;       // s
  Fixed bestBidPrice;       // b
  Fixed bestBidQty;         // B
  Fixed bestAskPrice;       // a
  Fixed bestAskQty;         // A
  uint64_t transactionTime; // T
  uint64_t eventTime;       // E
};

using ParsedMarketData = std::variant<TradeData, AggTradeData, MarkPriceData,
                                      DepthData, BookTickerData, ErrorParse>;

class TradeDataParser {
public:
  static ParsedMarketData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view TRADE_TIME = "T";
  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view TRADE_ID = "t";
  static constexpr std::string_view PRICE = "p";
  static constexpr std::string_view QUANTITY = "q";
  static constexpr std::string_view ORDER_T = "X";
  static constexpr std::string_view MARKET_MAKER = "m";
};

class AggTradeDataParser {
public:
  static ParsedMarketData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view AGG_TRADE_ID = "a";
  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view PRICE = "p";
  static constexpr std::string_view QUANTITY = "q";
  static constexpr std::string_view NORMAL_QUANTITY = "nq";
  static constexpr std::string_view FIRST_ORDER_ID = "f";
  static constexpr std::string_view LAST_ORDER_ID = "l";
  static constexpr std::string_view TRADE_TIME = "T";
  static constexpr std::string_view MARKET_MAKER = "m";
};

class MarkPriceDataParser {
public:
  static ParsedMarketData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view MARK_PRICE = "p";
  static constexpr std::string_view SETTLE_PRICE = "P";
  static constexpr std::string_view INDEX_PRICE = "i";
  static constexpr std::string_view FUNDING_RATE = "r";
  static constexpr std::string_view NEXT_FUNDING_RATE = "T";
};

class DepthDataParser {
public:
  static ParsedMarketData parse(const StreamMessage &msg);

private:
  static bool parsePriceQty(const nlohmann::json &entry, Fixed &price,
                            Fixed &qty);

private:
  static constexpr std::string_view EVENT_TIME = "E";
  static constexpr std::string_view TRANSACTION_TIME = "T";
  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view FIRST_UPDT_ID = "U";
  static constexpr std::string_view FINAL_UPDT_ID = "u";
  static constexpr std::string_view FINAL_UPDT_ID_LAST_STREAM = "pu";
  static constexpr std::string_view BIDS = "b";
  static constexpr std::string_view ASKS = "a";
};

class BookTickerDataParser {
public:
  static ParsedMarketData parse(const StreamMessage &msg);

private:
  static constexpr std::string_view ORDER_BOOK_ID = "u";
  static constexpr std::string_view SYMBOL = "s";
  static constexpr std::string_view BID_PRICE = "b";
  static constexpr std::string_view BID_QTY = "B";
  static constexpr std::string_view ASK_PRICE = "a";
  static constexpr std::string_view ASK_QTY = "A";
  static constexpr std::string_view TRANSACTION_TIME = "T";
  static constexpr std::string_view EVENT_TIME = "E";
};

// Main Parser
class MarketDataParser {
public:
  static ParsedMarketData parse(const StreamMessage &msg);

private:
  static MARKET_DATA_TYPE _detect_msg_type(const StreamMessage &msg);
};

} // namespace Market

#endif // MARKET_DATA_PARSER_HPP

/*

 {"stream":"btcusdt@markPrice@1s","data":{"e":"markPriceUpdate","E":1771263510000,"s":"BTCUSDT","p":"68005.80802899","P":"67836.82194783","i":"6803
5.14043478","r":"-0.00004435","T":1771286400000}}

 {"stream":"btcusdt@trade","data":{"e":"trade","E":1771263510064,"T":1771263510064,"s":"BTCUSDT","t":7297376207,"p":"68005.00","q":"0.100","X":"MAR
KET","m":true}}

 {"stream":"btcusdt@aggTrade","data":{"e":"aggTrade","E":1771263510195,"a":3141966230,"s":"BTCUSDT","p":"68005.00","q":"2.695","nq":"2.695","f":729
7376181,"l":7297376207,"T":1771263510041,"m":true}}

 {"stream":"btcusdt@bookTicker","data":{"e":"bookTicker","u":9924360161244,"s":"BTCUSDT","b":"68005.00","B":"2.394","a":"68005.10","A":"0.857","T":
1771263510337,"E":1771263510337}}

{"stream":"btcusdt@depth5@100ms","data":{"e":"depthUpdate","E":1771263510140,"T":1771263510139,"s":"BTCUSDT","U":9924360134447,"u":9924360146998,"
pu":9924360134267,"b":[["68005.00","2.214"],["68004.90","0.042"],["68004.80","0.047"],["68004.70","0.061"],["68004.50","0.003"]],"a":[["68005.10",
"0.838"],["68005.20","0.218"],["68005.30","0.010"],["68005.40","0.007"],["68005.50","0.042"]]}}

{"stream":"btcusdt@depth@100ms","data":{"e":"depthUpdate","E":1771263510383,"T":1771263510381,"s":"BTCUSDT","U":9924360157613,"u":9924360163987,"p
u":9924360157549,"b":[["1000.00","33.061"],["5000.00","46.423"],["17505.00","0.924"],["17605.00","0.264"],["17705.00","1.848"],["17805.00","0.264"
],["37805.00","0.093"],["37905.00","0.207"],["67209.30","3.485"],["67324.90","0.764"],["67665.00","1.051"],["67767.80","0.006"],["67853.10","0.159
"],["67891.90","0.992"],["67893.10","0.746"],["67903.90","0.355"],["67917.10","0.071"],["67919.70","0.392"],["67932.00","0.048"],["67955.10","0.18
9"],["67955.40","0.008"],["67971.00","0.070"],["67972.70","2.350"],["67972.80","0.008"],["67973.30","0.073"],["67973.40","0.048"],["67974.50","0.0
09"],["67974.60","0.000"],["67975.10","0.004"],["67975.20","0.008"],["67979.10","0.002"],["67981.90","0.007"],["67982.00","0.112"],["67983.00","0.
003"],["67984.00","0.059"],["67984.50","2.320"],["67995.70","0.045"],["68005.00","2.344"]],"a":[["68005.10","0.857"],["68016.80","0.004"],["68017.
80","0.088"],["68020.10","0.000"],["68023.10","0.132"],["68027.20","0.025"],["68029.50","0.002"],["68030.40","0.149"],["68034.00","1.282"],["68034
.10","0.511"],["68034.60","0.044"],["68034.70","0.116"],["68039.10","0.053"],["68045.10","0.121"],["68049.70","0.034"],["68052.20","0.012"],["6805
2.30","0.712"],["68081.50","0.328"],["68137.40","0.257"],["68220.40","0.007"],["68685.10","0.000"],["68685.20","0.775"],["69344.80","0.132"]]}}

 */
