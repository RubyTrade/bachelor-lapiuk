#ifndef MARKET_STREAM_HPP
#define MARKET_STREAM_HPP

#include "net/net.hpp"
#include "utils/json.hpp"
#include "utils/queue.hpp"

#include <array>
#include <memory>
#include <string>
#include <string_view>

/*
 * SUBSCRIBE method example:
 * {{method", "SUBSCRIBE"},
 * params", {"pepeusdt@trade", "dogeusdt@trade"}},
 * {"id", "1"}}
 */
enum class MARKET_STREAM_METHOD {
  SUBSCRIBE = 0,
  UNSUBSCRIBE,
  LIST_SUBSCRIPTIONS,
  SET_PROPERTY,
  GET_PROPERTY
};

class StreamQueryBuilder {
public:
  enum class DEPTH_LEVELS { SMALL = 5, MID = 10, LARGE = 20 };

public:
  explicit StreamQueryBuilder(MARKET_STREAM_METHOD method);
  StreamQueryBuilder() = delete;

  std::optional<JSONQuery> commit();

  StreamQueryBuilder &add_trade_symbol(const std::string &symbol);
  StreamQueryBuilder &add_aggTrade_symbol(const std::string &symbol);
  StreamQueryBuilder &add_deffDepth_symbol(const std::string &symbol,
                                           bool fast_update = true);
  StreamQueryBuilder &add_partDepth_symbol(const std::string &symbol,
                                           DEPTH_LEVELS level,
                                           bool fast_update = true);
  StreamQueryBuilder &add_bookTicker_symbol(const std::string &symbol);

  StreamQueryBuilder &set_combined_property(bool combined_flag = true);

private:
  bool _is_query_valid();
  std::string _stream_method_to_str() const;
  void _add_to_params(const std::string &value);

private:
  static constexpr short MAX_PROPS_NUM = 3;
  static constexpr std::string_view PARAMS = "params";
  static constexpr std::string_view METHOD = "method";
  static constexpr std::string_view ID = "id";
  static constexpr std::array<std::string_view, 5> METHOD_STRINGS{
      "SUBSCRIBE", "UNSUBSCRIBE", "LIST_SUBSCRIPTIONS", "SET_PROPERTY",
      "GET_PROPERTY"};
  static constexpr std::array<
      std::pair<MARKET_STREAM_METHOD,
                std::array<std::string_view, MAX_PROPS_NUM>>,
      5>
      METHOD_REQUIREMENTS{
          {{MARKET_STREAM_METHOD::SUBSCRIBE, {PARAMS, METHOD, ID}},
           {MARKET_STREAM_METHOD::UNSUBSCRIBE, {PARAMS, METHOD, ID}},
           {MARKET_STREAM_METHOD::LIST_SUBSCRIPTIONS, {METHOD, ID, ""}},
           {MARKET_STREAM_METHOD::GET_PROPERTY, {PARAMS, METHOD, ID}},
           {MARKET_STREAM_METHOD::SET_PROPERTY, {PARAMS, METHOD, ID}}}};

private:
  MARKET_STREAM_METHOD m_method;
  std::string m_methodStr;

  static int s_requestId;

  JSONQuery m_query;

  std::string m_lastError{};
};

// Market Stream
class MarketStream {
public:
  MarketStream();

  WSError connect_to_websocket();

  WSError execute_query(const JSONQuery &query);

  // Temp method
  void start_listening();
  void start_reading();

private:
  Queue<std::string> m_msgQueue;

  std::unique_ptr<WebSocket> m_webSocket; // main stream socket
};

#endif // MARKET_STREAM_HPP
