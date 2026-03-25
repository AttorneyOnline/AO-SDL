/**
 * @file IConfiguration.h
 * @brief Base class for all application configurations.
 * @defgroup configuration Configuration System
 * @{
 */
#pragma once

#include <algorithm>
#include <any>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/// Pure interface for all configurations.
class IConfiguration {
  public:
    virtual ~IConfiguration() = default;

    using ChangeCallback = std::function<void(const std::string& key)>;

    virtual bool deserialize(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual void set_value(const std::string& key, const std::any& value) = 0;
    virtual std::any value(const std::string& key, const std::any& default_value = {}) const = 0;
    virtual bool contains(const std::string& key) const = 0;
    virtual void remove(const std::string& key) = 0;
    virtual void clear() = 0;

    using KeyValueVisitor = std::function<void(const std::string& key, const std::any& value)>;

    /// Return all keys in the configuration.
    virtual std::vector<std::string> keys() const = 0;

    /// Invoke @p visitor for every key-value pair.
    virtual void for_each(const KeyValueVisitor& visitor) const = 0;

    template <typename T>
    T value(const std::string& key, const T& default_value = T{}) const {
        const std::any result = value(key, std::any{default_value});
        if (result.has_value() && result.type() == typeid(T))
            return std::any_cast<T>(result);
        return default_value;
    }
};

// CRTP base that adds singleton access, thread-safe NVI wrappers, and a
// change callback on top of IConfiguration.
//
// The public IConfiguration virtuals are overridden as final here.  Each one
// acquires the appropriate lock, delegates to a protected do_* hook, and
// (for mutating operations) fires the change callback after the lock is
// released.  Subclasses only implement the do_* methods:
//
//   class MyConfig : public ConfigurationBase<MyConfig> {
//     protected:
//       void do_set_value(const std::string& key, const std::any& v) override {
//           map_[key] = v;
//       }
//       std::any do_value(const std::string& key,
//                         const std::any& def) const override {
//           auto it = map_.find(key);
//           return it != map_.end() ? it->second : def;
//       }
//   };
template <typename Derived>
class ConfigurationBase : public IConfiguration {
  public:
    ConfigurationBase(const ConfigurationBase&) = delete;
    ConfigurationBase& operator=(const ConfigurationBase&) = delete;

    using IConfiguration::value; // unhide the value<T> template

    static Derived& instance() {
        static_assert(std::is_base_of_v<ConfigurationBase, Derived>,
                      "Derived must inherit from ConfigurationBase");
        static Derived inst;
        return inst;
    }

    /// Register a global change callback that fires for every key.
    /// Returns an ID that can be passed to remove_on_change().
    int add_on_change(ChangeCallback cb) {
        std::unique_lock lock(mutex_);
        int id = next_cb_id_++;
        on_change_callbacks_.push_back({id, {}, std::move(cb)});
        return id;
    }

    /// Register a change callback that only fires when @p key is modified.
    /// Returns an ID that can be passed to remove_on_change().
    int add_on_change(const std::string& key, ChangeCallback cb) {
        std::unique_lock lock(mutex_);
        int id = next_cb_id_++;
        on_change_callbacks_.push_back({id, key, std::move(cb)});
        return id;
    }

    /// Remove a previously registered callback by its ID.
    void remove_on_change(int id) {
        std::unique_lock lock(mutex_);
        auto& cbs = on_change_callbacks_;
        cbs.erase(std::remove_if(cbs.begin(), cbs.end(),
                                 [id](const auto& entry) { return entry.id == id; }),
                  cbs.end());
    }

    /// Remove all registered change callbacks.
    void clear_on_change() {
        std::unique_lock lock(mutex_);
        on_change_callbacks_.clear();
    }

    // Thread-safe NVI overrides

    bool deserialize(const std::vector<uint8_t>& data) final {
        {
            std::unique_lock lock(mutex_);
            if (!do_deserialize(data))
                return false;
        }
        notify({});
        return true;
    }

    std::vector<uint8_t> serialize() const final {
        std::shared_lock lock(mutex_);
        return do_serialize();
    }

    void set_value(const std::string& key, const std::any& value) final {
        {
            std::unique_lock lock(mutex_);
            do_set_value(key, value);
        }
        notify(key);
    }

    std::any value(const std::string& key, const std::any& default_value = {}) const final {
        std::shared_lock lock(mutex_);
        return do_value(key, default_value);
    }

    bool contains(const std::string& key) const final {
        std::shared_lock lock(mutex_);
        return do_contains(key);
    }

    void remove(const std::string& key) final {
        {
            std::unique_lock lock(mutex_);
            do_remove(key);
        }
        notify(key);
    }

    void clear() final {
        {
            std::unique_lock lock(mutex_);
            do_clear();
        }
        notify({});
    }

    std::vector<std::string> keys() const final {
        std::shared_lock lock(mutex_);
        return do_keys();
    }

    void for_each(const KeyValueVisitor& visitor) const final {
        std::shared_lock lock(mutex_);
        do_for_each(visitor);
    }

  protected:
    ConfigurationBase() = default;

    // Subclasses implement these.  The mutex is already held when called.
    virtual bool do_deserialize(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> do_serialize() const = 0;
    virtual void do_set_value(const std::string& key, const std::any& value) = 0;
    virtual std::any do_value(const std::string& key, const std::any& default_value) const = 0;
    virtual bool do_contains(const std::string& key) const = 0;
    virtual void do_remove(const std::string& key) = 0;
    virtual void do_clear() = 0;
    virtual std::vector<std::string> do_keys() const = 0;
    virtual void do_for_each(const KeyValueVisitor& visitor) const = 0;

  private:
    struct SettingsCallbackEntry {
        int id;
        std::string key_filter; // empty = global (fires for every change)
        ChangeCallback cb;
    };

    void notify(const std::string& key) {
        std::vector<SettingsCallbackEntry> callbacks;
        {
            std::shared_lock lock(mutex_);
            callbacks = on_change_callbacks_;
        }
        for (const auto& entry : callbacks) {
            if (!entry.cb)
                continue;
            // Global callbacks (empty filter) always fire.
            // Key-specific callbacks fire only on exact match, or when key
            // is empty (bulk operations like clear/deserialize).
            if (entry.key_filter.empty() || key.empty() || entry.key_filter == key)
                entry.cb(key);
        }
    }

    mutable std::shared_mutex mutex_;
    std::vector<SettingsCallbackEntry> on_change_callbacks_;
    int next_cb_id_ = 0;
};

/** @} */
