#ifndef TRADING_STREAM_CONTROLLER_HPP
#define TRADING_STREAM_CONTROLLER_HPP

#include "core/controllers/trading_stream_utils.hpp"
#include "core/net/net.hpp"
#include "core/parsers/trading_stream_parser.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/utils/queue.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace Trading {

struct MessageStreams {
  // Main string message queue
  Queue<std::string> msgQueue;

  // Queue for parsed messages
  ObservableQueue<ResultMessage> resultsQueue;
};

class TradingStreamController {
public:
  TradingStreamController();

  std::string create_order(const TradeRequest &req);
  bool cancel_order(const TradeRequest &req);
  bool get_order_status(const TradeRequest &req);

  // TODO: support more requests
  // TODO: implement easy interface to get parsed data

private:
  void _start_listen_thread();
  void _start_read_thread();

  void _start_buffer_reading();

  NetError _do_session_logon();

  void _parse_msg(const std::string &&msg);

  TRADE_STREAM_METHOD _get_pending_method(const std::string &id);

private:
  // Main stream
  std::unique_ptr<TradingStream> m_tradeStream;

  std::unique_ptr<TradingStreamQueryBuilder> m_queryBuilder;
  std::unique_ptr<ParametersBuilder> m_paramBuilder;

  // Threads
  std::unique_ptr<Thread> m_listenThread;
  std::unique_ptr<Thread> m_readThread;

  // Main string message queue
  std::unique_ptr<MessageStreams> m_tradingMsgQueues;
  std::unique_ptr<Queue<ParsedTradingStream>> m_parsedTradingData;

  // key - req id
  std::unordered_map<std::string, TRADE_STREAM_METHOD> m_pendingRequests;
  std::mutex m_pendingReqMtx;
};

} // namespace Trading

#endif // TRADING_STREAM_CONTROLLER_HPP
