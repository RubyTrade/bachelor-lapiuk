#ifndef TRADING_STREAM_HPP
#define TRADING_STREAM_HPP

#include "core/utils/fixed_num.hpp"
#include "core/utils/json.hpp"
#include "stream.hpp"
#include <array>
#include <climits>
#include <optional>
#include <string>

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

/*
GTC - Good Till Cancel (GTC order valitidy is 1 year from placement)
IOC - Immediate or Cancel
FOK - Fill or Kill
GTX - Good Till Crossing (Post Only)
GTD - Good Till Date
RPI - Retail Price Improvement
*/

enum class TIME_IN_FORCE { GTC, IOC, FOK, GTX, GTD, RPI };

class ParametersBuilder {
public:
  std::optional<JSONQuery> commit();

  ParametersBuilder &add_symbol(const std::string &symbol);
  ParametersBuilder &add_positionSide(const POSITION_SIDE &pos_side);
  ParametersBuilder &add_price(const Fixed &price);
  ParametersBuilder &add_quantity(const Fixed &quantity);
  ParametersBuilder &add_side(const ORDER_SIDE &side);
  ParametersBuilder &add_timeInForce(const TIME_IN_FORCE &timeInForce);
  ParametersBuilder &add_type(const ORDER_TYPE &type);

  // Algo params
  ParametersBuilder &add_algoType(const ORDER_TYPE &type);
  ParametersBuilder &add_newOrderRespType(const std::string &type);
  ParametersBuilder &add_triggerPrice(const Fixed &price);

  // Additional params
  ParametersBuilder &add_orderId(int64_t orderId);
  ParametersBuilder &add_priceMatch(const std::string &priceMatch);
  ParametersBuilder &add_origType(const ORDER_TYPE &type);

private:
  void _add_to_params(const std::string &key, const std::string &value);

private:
  static constexpr std::array<EnumStringPair<POSITION_SIDE>, 3>
      POSITION_SIDE_STR{{{POSITION_SIDE::BOTH, "BOTH"},
                         {POSITION_SIDE::LONG, "LONG"},
                         {POSITION_SIDE::SHORT, "SHORT"}}};

  static constexpr std::array<EnumStringPair<ORDER_SIDE>, 2> ORDER_SIDE_STR{
      {{ORDER_SIDE::BUY, "BUY"}, {ORDER_SIDE::SELL, "SELL"}}};

  static constexpr std::array<EnumStringPair<TIME_IN_FORCE>, 6>
      TIME_IN_FORCE_STR{{
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

private:
  JSONQuery m_params_query;

  std::string m_lastError{};
};

/*
 * order.place method example:
{
    "id": "3f7df6e3-2df4-44b9-9919-d2f38f90a99a",
    "method": "order.place",
    "params": {
        "apiKey":
"HMOchcfii9ZRZnhjp2XjGXhsOBd6msAhKz9joQaWwZ7arcJTlD2hGPHQj1lGdTjR",
        "positionSide": "BOTH",
        "price": 43187.00,
        "quantity": 0.1,
        "side": "BUY",
        "symbol": "BTCUSDT",
        "timeInForce": "GTC",
        "timestamp": 1702555533821,
        "type": "LIMIT",
        "signature":
"0f04368b2d22aafd0ggc8809ea34297eff602272917b5f01267db4efbc1c9422"
    }
}
*/

enum class USER_DATA_STREAM_METHOD : uint16_t {
  INVALID_METHOD = 0x0000,
  SESSION_STATUS = 0x0001,
  SESSION_LOGOUT,

  SESSION_LOGON = 0x0100,
  ACCOUNT_STATUS,
  ACCOUNT_BALANCE,

  ORDER_PLACE = 0x0200,
  ORDER_MODIFY,
  ORDER_CANCEL,
  ORDER_STATUS,
  ALGO_ORDER_PLACE,
  ALGO_ORDER_CANCEL,
  ACCOUNT_POSITION,

  // TODO: Think about REST API methods
};

enum class USER_DATA_METHOD_GROUP : uint16_t {
  NO_PARAMS = 0x0001,
  PARAMS_ONLY = 0x0100,
  BOUNDLESS_PARAMS = 0x0200
};

class TradingStreamQueryBuilder {
/* Static API */ public:
  static bool is_query_valid(const JSONQuery &query);

  static bool is_params_required(const USER_DATA_STREAM_METHOD &method);
  static bool
  is_boundless_params_required(const USER_DATA_STREAM_METHOD &method);

public:
  explicit TradingStreamQueryBuilder(USER_DATA_STREAM_METHOD method);
  TradingStreamQueryBuilder() = delete;

  std::optional<JSONQuery> commit();

  TradingStreamQueryBuilder &add_borderless_params(const JSONQuery &query);

private:
  bool _is_query_valid();
  void _add_to_params(const std::string &key, const std::string &value);

  void _add_apiKey();
  void _add_current_timestamp();
  void _add_rsa_signature();
  void _add_params_query();

private:
  static constexpr std::string_view PARAMS = "params";
  static constexpr std::string_view METHOD = "method";
  static constexpr std::string_view ID = "id";
  static constexpr std::string_view PARAM_APIKEY = "apiKey";
  static constexpr std::string_view PARAM_SIGNATURE = "signature";
  static constexpr std::string_view PARAM_TIMESTAMP = "timestamp";

  static constexpr std::array<std::string_view, 2> METHOD_REQUIREMENTS{METHOD,
                                                                       ID};

  static constexpr std::array<std::string_view, 3> PARAMS_REQUIREMENTS{
      PARAM_APIKEY, PARAM_SIGNATURE, PARAM_TIMESTAMP};

  static constexpr std::array<EnumStringPair<USER_DATA_STREAM_METHOD>, 13>
      METHOD_NAMES{{
          {USER_DATA_STREAM_METHOD::INVALID_METHOD, "invalid.method"},
          {USER_DATA_STREAM_METHOD::SESSION_STATUS, "session.status"},
          {USER_DATA_STREAM_METHOD::SESSION_LOGOUT, "session.logout"},
          {USER_DATA_STREAM_METHOD::SESSION_LOGON, "session.logon"},
          {USER_DATA_STREAM_METHOD::ORDER_PLACE, "order.place"},
          {USER_DATA_STREAM_METHOD::ORDER_MODIFY, "order.modify"},
          {USER_DATA_STREAM_METHOD::ORDER_CANCEL, "order.cancel"},
          {USER_DATA_STREAM_METHOD::ORDER_STATUS, "order.status"},
          {USER_DATA_STREAM_METHOD::ALGO_ORDER_PLACE, "algoOrder.place"},
          {USER_DATA_STREAM_METHOD::ALGO_ORDER_CANCEL, "algoOrder.cancel"},
          {USER_DATA_STREAM_METHOD::ACCOUNT_POSITION, "account.position"},
          {USER_DATA_STREAM_METHOD::ACCOUNT_STATUS, "account.status"},
          {USER_DATA_STREAM_METHOD::ACCOUNT_BALANCE, "account.balance"},
      }};

private:
  USER_DATA_STREAM_METHOD m_method;
  std::string m_methodStr;

  static int s_requestId;

  JSONQuery m_query;
  JSONQuery m_params_query;

  std::string m_lastError{};
};

// Trading Stream
class TradingStream : public Stream {
public:
  NetError connect_to_websocket() override;

  NetError execute_query(const JSONQuery &query) override;
};

#endif // TRADING_STREAM_HPP
