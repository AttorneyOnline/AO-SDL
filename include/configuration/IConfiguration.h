/**
 * @file IConfiguration.h
 * @brief Base class for all application configurations.
 * @defgroup configuration Configuration System
 * @{
 */
#pragma once

#include <any>
#include <functional>
#include <shared_mutex>
#include <string>
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
    using IConfiguration::value; // unhide the value<T> template

    static Derived& instance() {
        static Derived inst;
        return inst;
    }

    void set_on_change(ChangeCallback cb) {
        std::unique_lock lock(mutex_);
        on_change_ = std::move(cb);
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

  protected:
    // Subclasses implement these.  The mutex is already held when called.
    virtual bool do_deserialize(const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> do_serialize() const = 0;
    virtual void do_set_value(const std::string& key, const std::any& value) = 0;
    virtual std::any do_value(const std::string& key, const std::any& default_value) const = 0;
    virtual bool do_contains(const std::string& key) const = 0;
    virtual void do_remove(const std::string& key) = 0;
    virtual void do_clear() = 0;

  private:
    void notify(const std::string& key) {
        ChangeCallback cb;
        {
            std::shared_lock lock(mutex_);
            cb = on_change_;
        }
        if (cb)
            cb(key);
    }

    mutable std::shared_mutex mutex_;
    ChangeCallback on_change_;
};

/** @} */
