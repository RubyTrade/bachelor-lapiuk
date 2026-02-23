#ifndef TRADING_STREAM_UTILS_HPP
#define TRADING_STREAM_UTILS_HPP

#include "core/utils/constants.hpp"
#include "core/utils/fixed_num.hpp"
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

  ResultMessage(const std::string _id, uint16_t _code)
      : id(_id), http_code(_code) {}

  bool isSuccess() const { return http_code == MsgKeys::SUCCESS_CODE; }
};

struct TradeRequest {
  ORDER_SIDE order_side;
  POSITION_SIDE position_side;
  ORDER_TYPE order_type;
  std::string symbol;
  Fixed quantity;
  Fixed price;

  // Optional fields
  TIME_IN_FORCE timeInForce{TIME_IN_FORCE::GTC};
  bool reduceOnly = false;

private:
  std::string clientOrderId;

public:
  TradeRequest(ORDER_SIDE o_side, POSITION_SIDE p_side, ORDER_TYPE o_type,
               const std::string &s, const Fixed &qty, const Fixed &p = {0})
      : order_side(o_side), position_side(p_side), order_type(o_type),
        symbol(s), quantity(qty), price(p) {
    static int unique_id_counter = 0;
    auto now = std::chrono::system_clock::now();
    uint64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count();
    clientOrderId = symbol + "_" + type_to_str(POSITION_SIDE_STR, p_side) +
                    "_" + std::to_string(now_ms) + "_" +
                    std::to_string(++unique_id_counter);
  }

  void setOrderSide(ORDER_SIDE side) { order_side = side; };
  void setPositionSide(POSITION_SIDE side) { position_side = side; };
  void setOrderType(ORDER_TYPE type) { order_type = type; };
  void setOrderSide(std::string s) { symbol = s; };
  void setQuantity(const Fixed &q) { quantity = q; };
  void setTimeInForce(TIME_IN_FORCE tif) { timeInForce = tif; }
  void setPrice(const Fixed &p) { price = p; }
  void setReduceOnly(bool reduce) { reduceOnly = true; }

  std::string getClientOrderId() const { return clientOrderId; }

  bool operator==(const TradeRequest &other) const {
    return clientOrderId == other.getClientOrderId();
  }
};

} // namespace Trading

#endif // TRADING_STREAM_UTILS_HPP
