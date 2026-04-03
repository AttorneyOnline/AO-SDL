/**
 * @file Metrics.h
 * @brief Lock-free Counter and Gauge types with label support.
 *
 * Designed for high-frequency inline instrumentation. Counters and Gauges
 * use std::atomic so they can be incremented from any thread without locks.
 * MetricFamily<T> groups labelled instances under a single metric name.
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace metrics {

// -- Primitive metric types ---------------------------------------------------

/// Monotonically increasing counter (reset only on process restart).
class Counter {
  public:
    void inc() {
        value_.fetch_add(1, std::memory_order_relaxed);
    }
    void inc(uint64_t n) {
        value_.fetch_add(n, std::memory_order_relaxed);
    }
    double value() const {
        return static_cast<double>(value_.load(std::memory_order_relaxed));
    }

  private:
    std::atomic<uint64_t> value_{0};
};

/// Value that can go up or down (e.g. active connections, temperature).
class Gauge {
  public:
    void set(double v) {
        value_.store(v, std::memory_order_relaxed);
    }

    void inc() {
        add(1.0);
    }
    void dec() {
        add(-1.0);
    }
    void inc(double n) {
        add(n);
    }
    void dec(double n) {
        add(-n);
    }

    double value() const {
        return value_.load(std::memory_order_relaxed);
    }

  private:
    void add(double n) {
        double old = value_.load(std::memory_order_relaxed);
        while (!value_.compare_exchange_weak(old, old + n, std::memory_order_relaxed))
            ;
    }
    std::atomic<double> value_{0.0};
};

// -- Label key for metric families --------------------------------------------

/// Joins label values into a single map key. Uses null byte as separator
/// (safe because label values are user-visible strings, never contain \0).
inline std::string join_labels(const std::vector<std::string>& values) {
    std::string key;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0)
            key += '\0';
        key += values[i];
    }
    return key;
}

// -- MetricFamily — labelled metric collection --------------------------------

/// Type-erased base for polymorphic iteration during serialization.
class MetricFamilyBase {
  public:
    virtual ~MetricFamilyBase() = default;

    const std::string& name() const {
        return name_;
    }
    const std::string& help() const {
        return help_;
    }
    const std::vector<std::string>& label_names() const {
        return label_names_;
    }
    virtual const char* type_string() const = 0;

    /// Visitor callback: (label_values[], metric_value).
    using Visitor = std::function<void(const std::vector<std::string>& label_values, double value)>;

    /// Iterate all labelled instances. Thread-safe (shared lock).
    virtual void visit(Visitor fn) const = 0;

  protected:
    MetricFamilyBase(std::string name, std::string help, std::vector<std::string> label_names)
        : name_(std::move(name)), help_(std::move(help)), label_names_(std::move(label_names)) {
    }

    std::string name_;
    std::string help_;
    std::vector<std::string> label_names_;
};

/// A family of labelled metrics of type T (Counter or Gauge).
template <typename T>
class MetricFamily : public MetricFamilyBase {
  public:
    MetricFamily(std::string name, std::string help, std::vector<std::string> label_names)
        : MetricFamilyBase(std::move(name), std::move(help), std::move(label_names)) {
    }

    /// Get or create a metric instance for the given label values.
    /// First call for a new label combo takes exclusive lock; subsequent calls
    /// find the cached entry under shared lock. Callers should cache the
    /// returned reference (e.g. in a static local) for hot-path access.
    T& labels(std::initializer_list<std::string> values) {
        return labels(std::vector<std::string>(values));
    }

    T& labels(const std::vector<std::string>& values) {
        std::string key = join_labels(values);

        // Fast path: shared lock read
        {
            std::shared_lock lock(mutex_);
            auto it = instances_.find(key);
            if (it != instances_.end())
                return *it->second;
        }

        // Slow path: exclusive lock, create new instance
        std::unique_lock lock(mutex_);
        auto& ptr = instances_[key];
        if (!ptr) {
            ptr = std::make_unique<T>();
            label_values_[key] = values;
        }
        return *ptr;
    }

    /// Convenience for label-less metrics.
    T& get() {
        return labels({});
    }

    const char* type_string() const override;

    void visit(Visitor fn) const override {
        std::shared_lock lock(mutex_);
        for (auto& [key, metric] : instances_) {
            auto lv_it = label_values_.find(key);
            if (lv_it != label_values_.end())
                fn(lv_it->second, metric->value());
        }
    }

  private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<T>> instances_;
    std::unordered_map<std::string, std::vector<std::string>> label_values_;
};

// Type string specializations
template <>
inline const char* MetricFamily<Counter>::type_string() const {
    return "counter";
}
template <>
inline const char* MetricFamily<Gauge>::type_string() const {
    return "gauge";
}

using CounterFamily = MetricFamily<Counter>;
using GaugeFamily = MetricFamily<Gauge>;

} // namespace metrics
