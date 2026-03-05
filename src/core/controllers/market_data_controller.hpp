#ifndef MARKET_DATA_CONTROLLER_HPP
#define MARKET_DATA_CONTROLLER_HPP

#include "core/controllers/market_data_utils.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/stream/market_stream.hpp"
#include "core/utils/json.hpp"
#include "core/utils/queue.hpp"
#include "core/utils/thread.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <termios.h>
#include <unordered_map>
#include <vector>

namespace Market {

class SubscriptionsList {
public:
  std::vector<MarketRequest> get_list() const;
  void add_to_list(const MarketRequest &req);
  void remove_from_list(const MarketRequest &req);
  void clear_the_list();

private:
  mutable std::mutex m_mtx;
  std::vector<MarketRequest> m_list;
};

struct MessageStreams {
  // Main string message queue
  Queue<std::string> msgQueue;

  // Queue for parsed messages
  ObservableQueue<StreamMessage> streamQueue;
  ObservableQueue<ResultMessage> resultQueue;
  ObservableQueue<ErrorMessage> errorQueue;
  ObservableQueue<UnknownMessage> unknownQueue;
};

class MarketDataController {
public:
  MarketDataController();

  bool subscribe_to(const MarketRequest &req);
  bool unsubscribe_from(const MarketRequest &req);
  std::vector<MarketRequest> get_list_of_subscriptions();

  // TODO: implement easy interface to get parsed data

private:
  void _reconnect();

  void _start_listen_thread();
  void _start_read_thread();

  void _start_buffer_reading();

  void _parse_msg(const std::string &&msg);

  MESSAGE_TYPE _detect_type(const JSONQuery &msg);

  void _fulfill_pending_result(const ResultMessage &msg);

  void _resubscribe_to_list(const std::vector<MarketRequest> &list);

private:
  // Main stream
  std::unique_ptr<MarketStream> m_marketStream;

  std::unique_ptr<MarketStreamQueryBuilder> m_queryBuilder;

  // Stream Market Data queues
  std::unique_ptr<Queue<ParsedMarketData>> m_parsedStreamData;
  std::unique_ptr<MessageStreams> m_marketMsgQueues;

  // Threads
  std::unique_ptr<Thread> m_listenThread;
  std::unique_ptr<Thread> m_readThread;

  // Async subscriptions
  SubscriptionsList m_subList;

  std::unordered_map<std::string,
                     std::pair<MarketRequest, MARKET_STREAM_METHOD>>
      m_pendingReq;
  std::mutex m_pendingReqMtx;

  std::atomic_bool m_is_stream_running{false};
};

} // namespace Market

#endif // MARKET_DATA_CONTROLLER_HPP
