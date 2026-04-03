/**
 * @file EventChannel.h
 * @brief Thread-safe typed event queue.
 * @ingroup events
 */
#pragma once

#include "Event.h"

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

/**
 * @brief A thread-safe, typed FIFO queue for events of a single type.
 * @ingroup events
 *
 * @tparam T The event type. Must derive from Event.
 *
 * Every public method acquires an internal mutex, so all operations are
 * safe to call concurrently from multiple threads. The channel is
 * non-copyable but movable.
 */
template <typename T>
class EventChannel {
    static_assert(std::is_base_of<Event, T>::value, "EventChannel only supports Event objects");

  public:
    /** @brief Default constructor. Creates an empty channel. */
    EventChannel() = default;

    /** @brief Deleted copy constructor (channels are non-copyable). */
    EventChannel(const EventChannel&) = delete;
    /** @brief Deleted copy assignment (channels are non-copyable). */
    EventChannel& operator=(const EventChannel&) = delete;

    /** @brief Default move constructor. */
    EventChannel(EventChannel&&) = default;
    /** @brief Default move assignment operator. */
    EventChannel& operator=(EventChannel&&) = default;

    /**
     * @brief Publishes an event to the back of the queue.
     *
     * Acquires the internal mutex for the duration of the call.
     * If a notify callback is set, it is invoked after the event is enqueued
     * (outside the lock).
     *
     * @param ev The event to enqueue (moved into the queue).
     */
    void publish(T&& ev) {
        std::shared_ptr<std::function<void()>> cb;
        {
            const std::lock_guard<std::mutex> lock(event_queue_mutex);
            event_queue.push_back(std::move(ev));
            ++publish_count_;
            cb = on_publish_; // shared_ptr copy is cheap (refcount bump, no allocation)
        }
        if (cb)
            (*cb)();
    }

    /**
     * @brief Set a callback invoked after each publish().
     *
     * The callback is invoked outside the channel's lock, so it is safe
     * to call poller.notify() or other non-reentrant operations.
     * Only one callback is supported; setting a new one replaces the old.
     *
     * @param cb Callback to invoke, or nullptr to clear.
     */
    void set_on_publish(std::function<void()> cb) {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);
        if (cb)
            on_publish_ = std::make_shared<std::function<void()>>(std::move(cb));
        else
            on_publish_.reset();
    }

    /**
     * @brief Returns the total number of events published to this channel.
     * @return Monotonically increasing count of published events.
     */
    uint64_t publish_count() const {
        return publish_count_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Returns the number of pending events in the queue.
     *
     * Acquires the internal mutex for the duration of the call.
     *
     * @return The number of queued events.
     */
    size_t size() {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);
        return event_queue.size();
    }

    /**
     * @brief Checks whether the queue contains any pending events.
     *
     * Acquires the internal mutex for the duration of the call.
     *
     * @return @c true if at least one event is queued, @c false otherwise.
     */
    bool has_events() {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);
        return !event_queue.empty();
    }

    /**
     * @brief Dequeues and returns the oldest event, if any.
     *
     * Acquires the internal mutex for the duration of the call.
     *
     * @return The front event wrapped in std::optional, or std::nullopt
     *         if the queue is empty.
     */
    std::optional<T> get_event() {
        const std::lock_guard<std::mutex> lock(event_queue_mutex);
        if (event_queue.empty())
            return std::nullopt;
        T ev = std::move(event_queue.front());
        event_queue.pop_front();
        return ev;
    }

  private:
    std::mutex event_queue_mutex;                       /**< Guards all access to @c event_queue. */
    std::deque<T> event_queue;                          /**< FIFO storage for pending events. */
    std::atomic<uint64_t> publish_count_{0};            /**< Total events published (for debug stats). */
    std::shared_ptr<std::function<void()>> on_publish_; /**< Optional callback fired after each publish(). */
};
