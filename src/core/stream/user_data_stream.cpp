#include "user_data_stream.hpp"

#include "core/utils/constants.hpp"

#include <string>

UserDataStream::UserDataStream(Queue<std::string> &msgQueue)
    : Stream(msgQueue) {}

NetError UserDataStream::connect_to_websocket(const std::string &listenKey) {
  return Stream::_connect_to_websocket(
      std::string(Data::WS_HOST), Data::HTTPS_PORT,
      std::string(Data::WS_USERDATA_TARGET) + "/" + listenKey);
}

NetError UserDataStream::connect_to_websocket() {
  return NetError(NetErrorType::UNDEFINED,
                  "UserDataStream requires listenKey; use "
                  "connect_to_websocket(listenKey).");
}

NetError UserDataStream::execute_query(const JSONQuery &query) {
  (void)query;
  return NetError(NetErrorType::INVALID_QUERY_ERR,
                  "UserDataStream is read-only (listenKey push stream).");
}
