#ifndef EVENTCHANNEL_H
#define EVENTCHANNEL_H

#include "Event.h"

#include <mutex>
#include <optional>
#include <queue>
#include <type_traits>
#include <utility>

template <typename T>
class EventChannel {
    static_assert(std::is_base_of<Event, T>::value, "EventChannel only supports Event objects");

  public:
    EventChannel() = default;

    EventChannel(const EventChannel&) = delete;
    EventChannel& operator=(const EventChannel&) = delete;

    EventChannel(EventChannel&&) = default;
    EventChannel& operator=(EventChannel&&) = default;

    void publish(T&& ev) {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);

        event_queue.push(std::move(ev));
    }

    std::optional<T> get_event() {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);

        if (event_queue.empty()) {
            return std::nullopt;
        }

        T ev = std::move(event_queue.front());
        event_queue.pop();

        return ev;
    }

  private:
    std::mutex event_queue_mutex;

    std::queue<T> event_queue;
};

#endif