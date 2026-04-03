/**
 * @file MetricsRegistry.h
 * @brief Singleton registry that owns all metric families and drives collection.
 *
 * Register metrics at startup, then call collect() during Prometheus scrapes.
 * Snapshot collectors are invoked before serialization to populate point-in-time
 * gauges (e.g. session counts, memory usage).
 */
#pragma once

#include "metrics/Metrics.h"
#include "metrics/PrometheusFormatter.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace metrics {

class MetricsRegistry {
  public:
    static MetricsRegistry& instance() {
        static MetricsRegistry reg;
        return reg;
    }

    /// Register a counter family. Returns a reference for instrumentation.
    CounterFamily& counter(const std::string& name, const std::string& help,
                           std::vector<std::string> label_names = {}) {
        auto family = std::make_unique<CounterFamily>(name, help, std::move(label_names));
        auto& ref = *family;
        families_.push_back(std::move(family));
        return ref;
    }

    /// Register a gauge family. Returns a reference for instrumentation.
    GaugeFamily& gauge(const std::string& name, const std::string& help, std::vector<std::string> label_names = {}) {
        auto family = std::make_unique<GaugeFamily>(name, help, std::move(label_names));
        auto& ref = *family;
        families_.push_back(std::move(family));
        return ref;
    }

    /// Register a callback that populates snapshot gauges before each scrape.
    void add_collector(std::function<void()> fn) {
        collectors_.push_back(std::move(fn));
    }

    /// Collect all metrics, run snapshot collectors, and serialize.
    /// Serialized so concurrent scrapes don't produce garbled snapshots.
    std::string collect() {
        std::lock_guard lock(collect_mutex_);

        // Run snapshot collectors to populate point-in-time gauges
        for (auto& fn : collectors_)
            fn();

        // Serialize all families
        PrometheusFormatter fmt;
        std::string out;
        out.reserve(4096);
        for (auto& family : families_)
            fmt.format(*family, out);
        return out;
    }

  private:
    MetricsRegistry() = default;

    std::vector<std::unique_ptr<MetricFamilyBase>> families_;
    std::vector<std::function<void()>> collectors_;
    std::mutex collect_mutex_;
};

} // namespace metrics
