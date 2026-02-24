#ifndef USER_DATA_STREAM_HPP
#define USER_DATA_STREAM_HPP

#include "core/net/net.hpp"
#include "core/utils/json.hpp"
#include "stream.hpp"

// User Data Stream
class UserDataStream : public Stream {
public:
  UserDataStream(Queue<std::string> &msgQueue);

  NetError connect_to_websocket(const std::string &listenKey);

  void disconnect_from_websocket();

private:
  NetError connect_to_websocket() override;

  NetError execute_query(const JSONQuery &query) override;
};

#endif // USER_DATA_STREAM_HPP
