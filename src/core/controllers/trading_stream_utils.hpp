#ifndef TRADING_STREAM_UTILS_HPP
#define TRADING_STREAM_UTILS_HPP

#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
#include "core/utils/helper_utils.hpp"
#include "core/utils/json.hpp"

#include <chrono>
#include <cstdint>
#include <string>

namespace Trading {

enum class MESSAGE_TYPE { SUCCESS, ERROR, UNKNOWN };

namespace MsgKeys {
inline static constexpr std::string_view ID = "id";
inline static constexpr std::string_view STATUS = "status";
inline static constexpr std::string_view RESULT = "result";
inline static constexpr std::string_view ERROR = "error";

inline static constexpr std::string_view CODE = "code";
inline static constexpr std::string_view MSG = "msg";

inline static constexpr std::string_view DEFAULT_VAL = "unknown";

inline static constexpr uint16_t SUCCESS_CODE = 200;
} // namespace MsgKeys

struct ResultMessage {
  MESSAGE_TYPE type = MESSAGE_TYPE::UNKNOWN;
  std::string id;
  uint16_t http_code;

  int error_code = 0;
  std::string error_msg{};
  JSONQuery result_msg{};

  ResultMessage(const std::string &_id, uint16_t _code)
      : id(_id), http_code(_code) {}

  bool isSuccess() const { return type == MESSAGE_TYPE::SUCCESS; }
};

struct TradeRequest {
  ORDER_SIDE order_side;
  POSITION_SIDE position_side;
  ORDER_TYPE order_type;
  std::string symbol;
  Fixed quantity;
  Fixed price;

  // Optional fields
  Fixed stopPrice{0};
  TIME_IN_FORCE timeInForce{TIME_IN_FORCE::GTC};
  bool reduceOnly = false;
  bool closePosition = false;

private:
  std::string clientOrderId;
  std::string clientAlgoId;
  static int s_unique_id_counter;
  static int s_unique_algo_id_counter;

public:
  TradeRequest()
      : order_side(ORDER_SIDE::BUY), position_side(POSITION_SIDE::BOTH),
        order_type(ORDER_TYPE::MARKET), symbol({}), quantity(Fixed{0}),
        price(Fixed{0}) {
    clientOrderId = "";
    clientAlgoId = "";
  }

  TradeRequest(ORDER_SIDE o_side, POSITION_SIDE p_side, ORDER_TYPE o_type,
               const std::string &s, const Fixed &qty = {0},
               const Fixed &p = {0})
      : order_side(o_side), position_side(p_side), order_type(o_type),
        symbol(s), quantity(qty), price(p) {
    _generateClientOrderId();
    _generateClientAlgoId();
  }

  bool isValid() const { return !clientOrderId.empty(); }

  // TODO: decide if needed
  void setOrderSide(ORDER_SIDE side) { order_side = side; };
  void setPositionSide(POSITION_SIDE side) { position_side = side; };
  void setOrderType(ORDER_TYPE type) { order_type = type; };
  void setOrderSide(std::string s) { symbol = s; };
  void setQuantity(const Fixed &q) { quantity = q; };
  void setPrice(const Fixed &p) { price = p; }
  void setStopPrice(const Fixed &p) { stopPrice = p; }

  void setTimeInForce(TIME_IN_FORCE tif) { timeInForce = tif; }
  void setReduceOnly(bool reduce) { reduceOnly = true; }
  void setClosePosition(bool close) { closePosition = true; }

  std::string getClientOrderId() const { return clientOrderId; }
  std::string getClientAlgoId() const { return clientAlgoId; }

  void _generateClientOrderId() {
    auto now = std::chrono::system_clock::now();
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count();
    clientOrderId =
        symbol + "_" + type_to_str(POSITION_SIDE_STR, position_side) + "_" +
        std::to_string(now_ms) + "_" + type_to_str(ORDER_SIDE_STR, order_side) +
        "_" + std::to_string(++s_unique_id_counter);
  }

  /// Binance `clientAlgoId`: max 36 chars, ^[\.A-Z\:/a-z0-9_-]{1,36}$
  void _generateClientAlgoId() {
    auto now = std::chrono::system_clock::now();
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count();
    clientAlgoId = "a" + std::to_string(now_ms % 10000000000ULL) + "x" +
                   std::to_string(++s_unique_algo_id_counter % 100000);
    if (clientAlgoId.size() > 36)
      clientAlgoId.resize(36);
  }

  bool operator==(const TradeRequest &other) const {
    return clientOrderId == other.getClientOrderId();
  }
};

} // namespace Trading

#endif // TRADING_STREAM_UTILS_HPP
