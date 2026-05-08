#include "data_dispatcher.hpp"
#include "core/parsers/market_data_parser.hpp"
#include "core/utils/event_publisher.hpp"
#include "strategy/strategies/strategy.hpp"
#include <memory>

// MarketDataDispatcher
MarketDataDispatcher::MarketDataDispatcher()
    : m_eventQueue(std::make_unique<Queue<Market::ParsedMarketData>>()),
      m_processingThread(std::make_unique<Thread>()) {}

MarketDataDispatcher::~MarketDataDispatcher() { stop(); }

void MarketDataDispatcher::enqueue(Market::ParsedMarketData event) {
  if (!std::holds_alternative<ErrorParse>(event))
    m_eventQueue->push_message(std::move(event));

  return;
}

void MarketDataDispatcher::start() {
  m_isQueueActive = true;
  m_processingThread->start(&MarketDataDispatcher::_listenToUpdates, this);
}

void MarketDataDispatcher::stop() {
  m_isQueueActive = false;
  m_eventQueue->stop_queue();
  m_processingThread->stop();
}

void MarketDataDispatcher::subscribe_to_dispatcher(IStrategy *listener) {
  if (!listener)
    return;

  if (std::find(m_strategies.begin(), m_strategies.end(), listener) ==
      m_strategies.end()) {
    m_strategies.push_back(listener);
  }
}

void MarketDataDispatcher::unsubscribe_from_dispatcher(IStrategy *listener) {
  m_strategies.erase(
      std::remove(m_strategies.begin(), m_strategies.end(), listener),
      m_strategies.end());
}

void MarketDataDispatcher::_listenToUpdates() {
  while (m_isQueueActive) {
    Market::ParsedMarketData out_msg;
    if (!m_eventQueue->pop_message(out_msg))
      break;

    for (auto *s : m_strategies) {
      s->on_market_data(out_msg);
    }
  }
}
