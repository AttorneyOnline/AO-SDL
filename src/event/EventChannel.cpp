#include "EventChannel.h"

EventChannel::EventChannel() {
}

void EventChannel::publish(Event ev) {
    const std::lock_guard<std::mutex> lock(event_queue_mutex);
}

void subscribe(event_callback callback);
