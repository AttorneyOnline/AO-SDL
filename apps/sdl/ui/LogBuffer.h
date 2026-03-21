#pragma once

#include "utils/Log.h"

#include <deque>
#include <mutex>
#include <string>

/// Global log buffer that outlives any UI view.
/// Installs itself as a Log sink on first access and accumulates entries
/// from all threads. UI widgets read a snapshot via entries().
class LogBuffer {
  public:
    struct Entry {
        LogLevel level;
        std::string timestamp;
        std::string message;
    };

    static LogBuffer& instance() {
        static LogBuffer buf;
        return buf;
    }

    /// Thread-safe snapshot of all entries.
    std::deque<Entry> entries() const {
        std::lock_guard lock(mutex_);
        return entries_;
    }

    /// Clear all entries.
    void clear() {
        std::lock_guard lock(mutex_);
        entries_.clear();
    }

  private:
    LogBuffer() {
        Log::set_sink([this](LogLevel level, const std::string& ts, const std::string& msg) {
            std::lock_guard lock(mutex_);
            entries_.push_back({level, ts, msg});
            if ((int)entries_.size() > MAX_ENTRIES)
                entries_.pop_front();
        });
    }

    static constexpr int MAX_ENTRIES = 5000;
    mutable std::mutex mutex_;
    std::deque<Entry> entries_;
};
