#ifndef TRADING_STREAM_CONTROLLER_HPP
#define TRADING_STREAM_CONTROLLER_HPP

#include "core/controllers/trading_stream_utils.hpp"
#include "core/net/net.hpp"
#include "core/stream/trading_stream.hpp"
#include "core/utils/queue.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace Trading {

/*
class RequestsList {
public:
  std::vector<TradeRequest> get_list() const;
  void add_to_list(const TradeRequest &req);
  void remove_from_list(const TradeRequest &req);

private:
  mutable std::mutex m_mtx;
  std::vector<TradeRequest> m_list;
};
*/

class TradingStreamController {
public:
  TradingStreamController();

  std::string create_order(const TradeRequest &req);
  bool cancel_order(const TradeRequest &req);
  bool get_order_status(const TradeRequest &req);

  // TODO: support more requests

private:
  void _start_listen_thread();
  void _start_read_thread();

  void _start_buffer_reading();

  NetError _do_session_logon();

  void _parse_msg(const std::string &&msg);

  void _fulfill_pending_result(const ResultMessage &msg);

private:
  // Main stream
  std::unique_ptr<TradingStream> m_tradeStream;

  std::unique_ptr<TradingStreamQueryBuilder> m_queryBuilder;
  std::unique_ptr<ParametersBuilder> m_paramBuilder;

  // Threads
  std::unique_ptr<Thread> m_listenThread;
  std::unique_ptr<Thread> m_readThread;

  // Main string message queue
  Queue<std::string> m_msgQueue;

  ObservableQueue<ResultMessage> m_resultsQueue;

  std::unordered_map<std::string, std::pair<TradeRequest, TRADE_STREAM_METHOD>>
      m_pendingRequests;
  std::mutex m_pendingReqMtx;
};

} // namespace Trading

#endif // TRADING_STREAM_CONTROLLER_HPP
