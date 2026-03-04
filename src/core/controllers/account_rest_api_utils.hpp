#ifndef ACCOUNT_REST_API_UTILS_HPP
#define ACCOUNT_REST_API_UTILS_HPP

#include "core/utils/constants.hpp"
#include "core/utils/json.hpp"

namespace AccountRestApi {

enum class ACCOUNT_API_TYPE {
  UNKNOWN,
  ACCOUNT_INFO,
  ACCOUNT_BALANCE,
  COMMISSION_RATE
};

struct RestApiMessage {
  ACCOUNT_API_TYPE apiType;
  JSONQuery data;

  RestApiMessage(ACCOUNT_API_TYPE type, const JSONQuery &json)
      : apiType(type), data(json) {}
};

} // namespace AccountRestApi

#endif // ACCOUNT_REST_API_UTILS_HPP
