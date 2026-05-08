#ifndef USER_DATA_STREAM_HPP
#define USER_DATA_STREAM_HPP

#include "core/net/net.hpp"
#include "core/utils/json.hpp"
#include "stream.hpp"

/// USD-M futures user-data push stream: `wss://fstream.binance.com/private/ws/<listenKey>`
/// (REST `POST /fapi/v1/listenKey`). `userDataStream.subscribe` is not supported on `ws-fapi/v1`.
class UserDataStream : public Stream {
public:
  UserDataStream(Queue<std::string> &msgQueue);

  NetError connect_to_websocket(const std::string &listenKey);

private:
  NetError connect_to_websocket() override;

  NetError execute_query(const JSONQuery &query) override;
};

#endif // USER_DATA_STREAM_HPP
