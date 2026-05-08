#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include "helper_utils.hpp"

#include <string_view>

enum class MARKET_DATA_TYPE {
  UNKNOWN,
  TRADE,
  AGG_TRADE,
  MARK_PRICE,
  DIFF_DEPTH,
  PART_DEPTH,
  BOOK_TICKER,
};

enum class ORDER_TYPE {
  LIMIT,
  MARKET,
  STOP,
  STOP_MARKET,
  TAKE_PROFIT,
  TAKE_PROFIT_MARKET,
  TRAILING_STOP_MARKET
};

enum class ORDER_SIDE { BUY, SELL };
enum class POSITION_SIDE { BOTH, LONG, SHORT };
enum class MARGIN_TYPE { CROSSED, ISOLATED };

/*
GTC - Good Till Cancel (GTC order valitidy is 1 year from placement)
IOC - Immediate or Cancel
FOK - Fill or Kill
GTX - Good Till Crossing (Post Only)
GTD - Good Till Date
RPI - Retail Price Improvement
*/

enum class TIME_IN_FORCE { GTC, IOC, FOK, GTX, GTD, RPI };

// TODO: add new events
enum class USER_DATA_EVENT_TYPE {
  LISTEN_KEY_EXPIRY,
  ACCOUNT_UPDATE,
  MARGIN_CALL,
  ORDER_TRADE_UPDATE,
  TRADE_LITE,
  ACCOUNT_CONFIG_UPDATE,
  /// Algo Service conditional order lifecycle (do not mis-map to LISTEN_KEY_EXPIRY).
  ALGO_UPDATE,
};

enum class EXECUTION_TYPE { NEW, CANCELED, CALCULATED, EXPIRED, TRADE };

enum class ORDER_STATUS {
  NEW,
  PARTIALLY_FILLED,
  FILLED,
  CANCELED,
  EXPIRED,
  REJECTED
};

enum class WORKING_TYPE { CONTRACT_PRICE, MARK_PRICE };

enum class SELF_TRADE_PREVENTION_MODE {
  NONE,
  EXPIRE_MAKER,
  EXPIRE_TAKER,
  EXPIRE_BOTH
};

enum class SIGNAL { HOLD, LONG_ENTRY, SHORT_ENTRY, CLOSE_LONG, CLOSE_SHORT };

enum class STRATEGIES { UNKNOWN, MEAN_REVERSION };

enum class ORDER_SIDE_TYPE { LONG, SHORT };

static constexpr std::array<EnumStringPair<ORDER_SIDE_TYPE>, 2>
    ORDER_SIDE_TYPE_STR{{
        {ORDER_SIDE_TYPE::LONG, "LONG"},
        {ORDER_SIDE_TYPE::SHORT, "SHORT"},
    }};

static constexpr std::array<EnumStringPair<STRATEGIES>, 2> STRATEGIES_STR{{
    {STRATEGIES::UNKNOWN, "UNKNOWN"},
    {STRATEGIES::MEAN_REVERSION, "MEAN_REVERSION"},
}};

static constexpr std::array<EnumStringPair<SIGNAL>, 5> SIGNAL_STR{{
    {SIGNAL::HOLD, "HOLD"},
    {SIGNAL::LONG_ENTRY, "LONG_ENTRY"},
    {SIGNAL::SHORT_ENTRY, "SHORT_ENTRY"},
    {SIGNAL::CLOSE_LONG, "CLOSE_LONG"},
    {SIGNAL::CLOSE_SHORT, "CLOSE_SHORT"},
}};

static constexpr std::array<EnumStringPair<USER_DATA_EVENT_TYPE>, 7>
    USER_DATA_EVENT_TYPE_STR{{
        {USER_DATA_EVENT_TYPE::LISTEN_KEY_EXPIRY, "listenKeyExpired"},
        {USER_DATA_EVENT_TYPE::ACCOUNT_UPDATE, "ACCOUNT_UPDATE"},
        {USER_DATA_EVENT_TYPE::MARGIN_CALL, "MARGIN_CALL"},
        {USER_DATA_EVENT_TYPE::ORDER_TRADE_UPDATE, "ORDER_TRADE_UPDATE"},
        {USER_DATA_EVENT_TYPE::TRADE_LITE, "TRADE_LITE"},
        {USER_DATA_EVENT_TYPE::ACCOUNT_CONFIG_UPDATE, "ACCOUNT_CONFIG_UPDATE"},
        {USER_DATA_EVENT_TYPE::ALGO_UPDATE, "ALGO_UPDATE"},
    }};

static constexpr std::array<EnumStringPair<SELF_TRADE_PREVENTION_MODE>, 4>
    SELF_TRADE_PREVENTION_MODE_STR{{
        {SELF_TRADE_PREVENTION_MODE::NONE, "NONE"},
        {SELF_TRADE_PREVENTION_MODE::EXPIRE_MAKER, "EXPIRE_MAKER"},
        {SELF_TRADE_PREVENTION_MODE::EXPIRE_TAKER, "EXPIRE_TAKER"},
        {SELF_TRADE_PREVENTION_MODE::EXPIRE_BOTH, "EXPIRE_BOTH"},
    }};

static constexpr std::array<EnumStringPair<EXECUTION_TYPE>, 5>
    EXECUTION_TYPE_STR{{
        {EXECUTION_TYPE::NEW, "NEW"},
        {EXECUTION_TYPE::CALCULATED, "CALCULATED"},
        {EXECUTION_TYPE::CANCELED, "CANCELED"},
        {EXECUTION_TYPE::EXPIRED, "EXPIRED"},
        {EXECUTION_TYPE::TRADE, "TRADE"},
    }};

static constexpr std::array<EnumStringPair<ORDER_STATUS>, 6> ORDER_STATUS_STR{{
    {ORDER_STATUS::NEW, "NEW"},
    {ORDER_STATUS::PARTIALLY_FILLED, "PARTIALLY_FILLED"},
    {ORDER_STATUS::FILLED, "FILLED"},
    {ORDER_STATUS::CANCELED, "CANCELED"},
    {ORDER_STATUS::EXPIRED, "EXPIRED"},
    {ORDER_STATUS::REJECTED, "REJECTED"},
}};

static constexpr std::array<EnumStringPair<WORKING_TYPE>, 2> WORKING_TYPE_STR{{
    {WORKING_TYPE::CONTRACT_PRICE, "CONTRACT_PRICE"},
    {WORKING_TYPE::MARK_PRICE, "MARK_PRICE"},
}};

static constexpr std::array<EnumStringPair<MARKET_DATA_TYPE>, 6>
    MARKET_DATA_TYPE_STR{{
        {MARKET_DATA_TYPE::TRADE, "trade"},
        {MARKET_DATA_TYPE::AGG_TRADE, "aggTrade"},
        {MARKET_DATA_TYPE::MARK_PRICE, "markPrice"},
        {MARKET_DATA_TYPE::BOOK_TICKER, "bookTicker"},
        {MARKET_DATA_TYPE::DIFF_DEPTH, "depth"},
        {MARKET_DATA_TYPE::PART_DEPTH, "depth"},
    }};

static constexpr std::array<EnumStringPair<POSITION_SIDE>, 3> POSITION_SIDE_STR{
    {{POSITION_SIDE::BOTH, "BOTH"},
     {POSITION_SIDE::LONG, "LONG"},
     {POSITION_SIDE::SHORT, "SHORT"}}};

static constexpr std::array<EnumStringPair<MARGIN_TYPE>, 2> MARGIN_TYPE_STR{
    {{MARGIN_TYPE::CROSSED, "cross"}, {MARGIN_TYPE::ISOLATED, "isolated"}}};

static constexpr std::array<EnumStringPair<ORDER_SIDE>, 2> ORDER_SIDE_STR{
    {{ORDER_SIDE::BUY, "BUY"}, {ORDER_SIDE::SELL, "SELL"}}};

static constexpr std::array<EnumStringPair<TIME_IN_FORCE>, 6> TIME_IN_FORCE_STR{
    {
        {TIME_IN_FORCE::GTC, "GTC"},
        {TIME_IN_FORCE::IOC, "IOC"},
        {TIME_IN_FORCE::FOK, "FOK"},
        {TIME_IN_FORCE::GTX, "TGX"},
        {TIME_IN_FORCE::GTD, "GTD"},
        {TIME_IN_FORCE::RPI, "RPI"},
    }};

static constexpr std::array<EnumStringPair<ORDER_TYPE>, 7> ORDER_TYPE_STR{
    {{ORDER_TYPE::MARKET, "MARKET"},
     {ORDER_TYPE::LIMIT, "LIMIT"},
     {ORDER_TYPE::STOP_MARKET, "STOP_MARKET"},
     {ORDER_TYPE::STOP, "STOP"},
     {ORDER_TYPE::TRAILING_STOP_MARKET, "TRAILING_STOP_MARKET"},
     {ORDER_TYPE::TAKE_PROFIT_MARKET, "TAKE_PROFIT_MARKET"},
     {ORDER_TYPE::TAKE_PROFIT, "TAKE_PROFIT"}}};

namespace Data {
inline static constexpr std::string_view BINANCE_READ_APIKEY_ENV =
    "BINANCE_READ_API_KEY";
inline static constexpr std::string_view BINANCE_WRITE_APIKEY_ENV =
    "BINANCE_WRITE_API_KEY";
inline static constexpr std::string_view BINANCE_PK_ENV = "BINANCE_PRIVATE_KEY";
inline static constexpr std::string_view BINANCE_READ_REST_APIKEY_ENV =
    "BINANCE_READ_REST_API_KEY";
inline static constexpr std::string_view BINANCE_READ_REST_SECRET_KEY_ENV =
    "BINANCE_READ_REST_SECRET_KEY";

inline constexpr std::string_view WS_HOST = "fstream.binance.com";
inline constexpr std::string_view WS_TRADING_HOST = "ws-fapi.binance.com";
inline constexpr std::string_view WS_DEFAULT_TARGET = "/stream";
/// USD-M user data stream path (requires REST listenKey). Docs: fstream + `/private/ws/<listenKey>`.
inline constexpr std::string_view WS_USERDATA_TARGET = "/private/ws";
inline constexpr std::string_view WS_TRADING_API_TARGET = "/ws-fapi/v1";

inline constexpr std::string_view API_HOST = "fapi.binance.com";

inline constexpr std::string_view API_DEFAULT_TARGET = "/fapi/v1/";

// REST API Endpoints
namespace ApiEndpoints {
inline constexpr std::string_view ACCOUNT_INFO = "/fapi/v2/account";
inline constexpr std::string_view ACCOUNT_BALANCE = "/fapi/v2/balance";
inline constexpr std::string_view COMMISSION_RATE = "/fapi/v1/commissionRate";
} // namespace ApiEndpoints

inline constexpr int WS_PORT_MAIN = 443;
inline constexpr int HTTPS_PORT = 443;

inline constexpr std::string_view HTTP_USER_AGENT = "RubyTradeHttpAgent";

namespace Header {
inline constexpr std::string_view APIKEY = "X-MBX-APIKEY";
inline constexpr std::string_view CONTENT_TYPE = "Content-Type";
inline constexpr std::string_view CONTENT_LENGTH = "Content-Length";
inline constexpr std::string_view CONNECTION = "Connection";
} // namespace Header
} // namespace Data

#endif // CONSTANTS_HPP
