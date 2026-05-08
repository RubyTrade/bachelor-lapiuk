#ifndef EVENT_PUBLISHER_HPP
#define EVENT_PUBLISHER_HPP

#include <algorithm>
#include <vector>

template <class Event> class IEventListener {
public:
  virtual ~IEventListener() = default;
  virtual void enqueue(Event event) = 0;
  virtual void start() = 0;
  virtual void stop() = 0;
};

template <class Event> class EventPublisher {
public:
  using IEventListenerType = IEventListener<Event>;

  void subscribe(IEventListenerType *listener) {
    if (!listener)
      return;

    if (std::find(m_listeners.begin(), m_listeners.end(), listener) ==
        m_listeners.end()) {
      m_listeners.push_back(listener);
      listener->start();
    }
  }

  void unsubscribe(IEventListenerType *listener) {
    m_listeners.erase(
        std::remove(m_listeners.begin(), m_listeners.end(), listener),
        m_listeners.end());
    listener->stop();
  }

  void publish(const Event &event) {
    auto listenersCopy = m_listeners;

    for (auto *listener : listenersCopy) {
      if (listener)
        listener->enqueue(event);
    }
  }

private:
  std::vector<IEventListenerType *> m_listeners;
};

#endif // EVENT_PUBLISHER_HPP
