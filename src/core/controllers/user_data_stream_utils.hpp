#ifndef USER_DATA_STREAM_UTILS_HPP
#define USER_DATA_STREAM_UTILS_HPP

#include "core/utils/constants.hpp"
#include "core/utils/json.hpp"

namespace UserData {

namespace MsgKeys {
inline static constexpr std::string_view EVENT_TYPE = "e";
} // namespace MsgKeys

struct StreamMessage {
  USER_DATA_EVENT_TYPE userDataType;
  JSONQuery data;

  StreamMessage(USER_DATA_EVENT_TYPE type, const JSONQuery &json)
      : userDataType(type), data(json) {}
};

} // namespace UserData

#endif // USER_DATA_STREAM_UTILS_HPP
