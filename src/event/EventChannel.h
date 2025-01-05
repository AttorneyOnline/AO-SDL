#pragma once

#include "Event.h"

#include <mutex>
#include <optional>
#include <deque>
#include <type_traits>
#include <utility>

// todo: we should probably store our event objects into a std::shared_ptr to manage the lifetime
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

    bool has_events(EventTarget target) {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);

        auto it = std::find_if(event_queue.begin(), event_queue.end(),
                               [target](const T& event) { return event.get_target() == target; });

        if (it != event_queue.end()) {
            return true;
        }
        else {
            return false;
        }
    }

    std::optional<T> get_event(EventTarget target) {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);

        auto it = std::find_if(event_queue.begin(), event_queue.end(),
                               [target](const T& event) { return event.get_target() == target; });

        if (it != event_queue.end()) {
            T ev = std::move(*it);
            event_queue.erase(it);
            return ev;
        }

        return std::nullopt;
    }

  private:
    std::mutex event_queue_mutex;

    std::deque<T> event_queue;
};
