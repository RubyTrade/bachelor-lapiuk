#ifndef DATA_DISPATCHER_HPP
#define DATA_DISPATCHER_HPP

#include "core/parsers/market_data_parser.hpp"
#include "core/utils/event_publisher.hpp"
#include "strategy/strategies/strategy.hpp"

#include <memory>

class MarketDataDispatcher : public IEventListener<Market::ParsedMarketData> {
public:
  MarketDataDispatcher();
  ~MarketDataDispatcher();

  void enqueue(Market::ParsedMarketData event) override;
  void stop() override;
  void start() override;

  void subscribe_to_dispatcher(IStrategy *listener);
  void unsubscribe_from_dispatcher(IStrategy *listener);

private:
  void _listenToUpdates();

private:
  std::unique_ptr<Queue<Market::ParsedMarketData>> m_eventQueue;
  std::atomic_bool m_isQueueActive{false};

  std::unique_ptr<Thread> m_processingThread;

  std::vector<IStrategy *> m_strategies;
};

#endif // DATA_DISPATCHER_HPP
