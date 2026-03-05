#include "user_data_stream.hpp"

#include "core/net/net.hpp"
#include "core/utils/constants.hpp"
#include "core/utils/json.hpp"
#include "stream.hpp"

#include <string>
#include <sys/types.h>

UserDataStream::UserDataStream(Queue<std::string> &msgQueue)
    : Stream(msgQueue) {}

NetError UserDataStream::connect_to_websocket(const std::string &listenKey) {
  NetError wsErr = Stream::_connect_to_websocket(
      std::string(Data::WS_HOST), Data::HTTPS_PORT,
      std::string(Data::WS_USERDATA_TARGET) + "/" + listenKey);
  return wsErr;
}

NetError UserDataStream::connect_to_websocket() {
  NetError wsErr{NetErrorType::UNDEFINED,
                 "connect_to_websocket should be passed with explicit "
                 "listenKey!"};
  return wsErr;
}

NetError UserDataStream::execute_query(const JSONQuery &query) {
  // UserDataStream is READ only!
  return NetError(NetErrorType::INVALID_QUERY_ERR,
                  "UserDataStream is READ only!");
}
