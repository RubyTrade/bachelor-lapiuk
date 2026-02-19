#ifndef MARKET_DATA_UTILS_HPP
#define MARKET_DATA_UTILS_HPP

#include "core/utils/json.hpp"
#include <string>

enum class MESSAGE_TYPE { STREAM, RESULT, ERROR, UNKNOWN };

namespace MsgKeys {
inline static constexpr std::string_view RESULT = "result";
inline static constexpr std::string_view RESULT_DATA = "id";
inline static constexpr std::string_view RESULT_SUCCESS = "null";

inline static constexpr std::string_view STREAM = "stream";
inline static constexpr std::string_view STREAM_DATA = "data";

inline static constexpr std::string_view ERROR = "code";
inline static constexpr std::string_view ERROR_DATA = "msg";

inline static constexpr std::string_view DEFAULT_VAL = "unknown";
} // namespace MsgKeys

struct IMarketMessage {
  MESSAGE_TYPE type;
  std::string_view keyStr;
};

struct StreamMessage : IMarketMessage {
  StreamMessage(const std::string &s, const JSONQuery &json)
      : IMarketMessage{MESSAGE_TYPE::STREAM, MsgKeys::STREAM}, stream(s),
        data(json) {}

  std::string stream;
  JSONQuery data;
};

struct ResultMessage : IMarketMessage {
  ResultMessage(const std::string &res, const std::string &id)
      : IMarketMessage{MESSAGE_TYPE::RESULT, MsgKeys::RESULT}, result(res),
        reqId(id) {
    isSuccessResult = (result == std::string(MsgKeys::RESULT_SUCCESS));
  }

  std::string result;
  std::string reqId;

  bool isSuccessResult;
};

struct ErrorMessage : IMarketMessage {
  ErrorMessage(const std::string &code, const std::string &err)
      : IMarketMessage{MESSAGE_TYPE::ERROR, MsgKeys::ERROR}, errCode(code),
        errMsg(err) {}

  std::string errCode;
  std::string errMsg;
};

struct UnknownMessage : IMarketMessage {
  UnknownMessage(const JSONQuery &json)
      : IMarketMessage{MESSAGE_TYPE::UNKNOWN, std::string_view{}}, data(json) {}

  JSONQuery data;
};

#endif // MARKET_DATA_UTILS_HPP
