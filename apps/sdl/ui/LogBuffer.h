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

    /// Copy only entries newer than the given generation into `out`.
    /// Returns the new generation watermark to pass next time.
    size_t poll(size_t since_gen, std::deque<Entry>& out) const {
        std::lock_guard lock(mutex_);
        if (generation_ > since_gen) {
            size_t new_entries = std::min(generation_ - since_gen, entries_.size());
            auto start = entries_.end() - (std::ptrdiff_t)new_entries;
            out.insert(out.end(), start, entries_.end());
            // Keep output bounded
            while (out.size() > MAX_ENTRIES)
                out.pop_front();
        }
        return generation_;
    }

    /// Clear all entries.
    void clear() {
        std::lock_guard lock(mutex_);
        entries_.clear();
        generation_ = 0;
    }

  private:
    LogBuffer() {
        Log::set_sink([this](LogLevel level, const std::string& ts, const std::string& msg) {
            std::lock_guard lock(mutex_);
            entries_.push_back({level, ts, msg});
            generation_++;
            if (entries_.size() > MAX_ENTRIES)
                entries_.pop_front();
        });
    }

    static constexpr size_t MAX_ENTRIES = 5000;
    mutable std::mutex mutex_;
    std::deque<Entry> entries_;
    size_t generation_ = 0;
};
