/**
 * @file EventManager.h
 * @brief Singleton, type-indexed registry of EventChannels.
 * @ingroup events
 */
#pragma once

#include "Event.h"
#include "EventChannel.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

/**
 * @brief Type-erased base for storing heterogeneous EventChannel instances.
 * @ingroup events
 */
class BaseEventChannel {
  public:
    /** @brief Virtual destructor for safe polymorphic deletion. */
    virtual ~BaseEventChannel() = default;
};

/**
 * @brief Typed wrapper that holds a single EventChannel<T> behind BaseEventChannel.
 * @ingroup events
 *
 * @tparam T The concrete Event subclass this wrapper manages.
 */
template <typename T>
class EventChannelWrapper : public BaseEventChannel {
  public:
    /** @brief Constructs the wrapper and allocates the underlying channel. */
    EventChannelWrapper() : channel(std::make_unique<EventChannel<T>>()) {
    }
    std::unique_ptr<EventChannel<T>> channel; /**< Owned EventChannel instance. */
};

/**
 * @brief Singleton that maps event types to their EventChannel instances.
 * @ingroup events
 *
 * Provides a single point of access for publishing and consuming events of
 * any type. Channels are created lazily on first access. All operations
 * acquire an internal mutex, making the manager safe to use from multiple
 * threads.
 *
 * Typical usage:
 * @code
 *   auto& ch = EventManager::instance().get_channel<ChatEvent>();
 *   ch.publish(ChatEvent("Alice", "Hello", false));
 * @endcode
 */
class EventManager {
  public:
    /**
     * @brief Returns the singleton EventManager instance.
     *
     * The instance is constructed on first call (Meyers' singleton).
     *
     * @return Reference to the global EventManager.
     */
    static EventManager& instance() {
        static EventManager instance;
        return instance;
    }

    /**
     * @brief Retrieves (or lazily creates) the EventChannel for event type @p T.
     *
     * Acquires the internal mutex for the duration of the call. If no channel
     * exists for @p T yet, one is created and stored before the reference is
     * returned.
     *
     * @tparam T A concrete class derived from Event.
     * @return Reference to the EventChannel<T> for the requested type.
     * @throws std::runtime_error If an internal type mismatch is detected
     *         (should not happen under normal usage).
     */
    template <typename T>
    EventChannel<T>& get_channel() {
        static_assert(std::is_base_of<Event, T>::value, "T must inherit from Event");

        std::type_index typeIdx(typeid(T));

        // Lock the mutex to ensure thread safety
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if the channel already exists
        auto it = channels_.find(typeIdx);
        if (it == channels_.end()) {
            // Create a new EventChannelWrapper for type T
            auto wrapper = std::make_unique<EventChannelWrapper<T>>();
            EventChannel<T>& channel_ref = *(wrapper->channel);

            // Insert the wrapper into the map
            channels_.emplace(typeIdx, std::move(wrapper));

            return channel_ref;
        }
        else {
            // Retrieve the existing wrapper and cast it to the appropriate type
            auto wrapper = dynamic_cast<EventChannelWrapper<T>*>(it->second.get());
            if (!wrapper) {
                throw std::runtime_error("Stored EventChannel type mismatch.");
            }
            return *(wrapper->channel);
        }
    }

  private:
    /** @brief Private constructor (singleton pattern). */
    EventManager() = default;

    /** @brief Deleted copy constructor. */
    EventManager(const EventManager&) = delete;
    /** @brief Deleted copy assignment. */
    EventManager& operator=(const EventManager&) = delete;

    /** @brief Type-indexed map of all active channels. */
    std::unordered_map<std::type_index, std::unique_ptr<BaseEventChannel>> channels_;
    /** @brief Guards all access to @c channels_. */
    std::mutex mutex_;
};
