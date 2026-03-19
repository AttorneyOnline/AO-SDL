#pragma once

#include "Event.h"

#include <mutex>
#include <optional>
#include <deque>
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
        event_queue.push_back(std::move(ev));
    }

    bool has_events() {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);
        return !event_queue.empty();
    }

    std::optional<T> get_event() {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);
        if (event_queue.empty()) return std::nullopt;
        T ev = std::move(event_queue.front());
        event_queue.pop_front();
        return ev;
    }

  private:
    std::mutex event_queue_mutex;
    std::deque<T> event_queue;
};
