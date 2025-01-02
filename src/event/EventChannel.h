#ifndef EVENTCHANNEL_H
#define EVENTCHANNEL_H

#include "Event.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

class EventLoop;

class EventChannel {
  public:
    EventChannel();

    typedef event_callback std::function<void(const Event&)>;

    void publish(Event ev);
    void subscribe(event_callback callback);

  private:
    friend class EventLoop;

    std::mutex event_queue_mutex;

    std::queue<Event> event_queue;
    std::vector<event_callback> subscribers;
};

#endif