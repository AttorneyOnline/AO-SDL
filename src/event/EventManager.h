#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "Event.h"
#include "EventChannel.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

// Base class for type-erased storage
class BaseEventChannel {
  public:
    virtual ~BaseEventChannel() = default;
};

// Wrapper to hold specific EventChannel instances
template <typename T>
class EventChannelWrapper : public BaseEventChannel {
  public:
    EventChannelWrapper() : channel(std::make_unique<EventChannel<T>>()) {
    }
    std::unique_ptr<EventChannel<T>> channel;
};

class EventManager {
  public:
    // Singleton access
    static EventManager& instance() {
        static EventManager instance;
        return instance;
    }

    // Template method to get the EventChannel for a specific Event type
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
    // Private constructor for singleton pattern
    EventManager() = default;

    // Delete copy and move semantics
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    // Storage for channels using type_index as the key
    std::unordered_map<std::type_index, std::unique_ptr<BaseEventChannel>> channels_;
    std::mutex mutex_;
};

#endif