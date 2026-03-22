#pragma once

#include "game/IScenePresenter.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

/// Reusable tick profiler that tracks timing for named sections.
/// Sections are registered once and timed with RAII scoped guards.
class TickProfiler {
  public:
    /// Register a named section. Returns the section index.
    int add_section(const char* name) {
        auto s = std::unique_ptr<Section>(new Section{name, {}});
        sections_.push_back(std::move(s));
        return static_cast<int>(sections_.size()) - 1;
    }

    /// RAII guard that measures a section's duration.
    class ScopedSection {
      public:
        ScopedSection(std::atomic<int>& counter) : counter_(counter), start_(std::chrono::steady_clock::now()) {
        }
        ~ScopedSection() {
            auto elapsed = std::chrono::steady_clock::now() - start_;
            counter_.store(static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()),
                           std::memory_order_relaxed);
        }

        ScopedSection(const ScopedSection&) = delete;
        ScopedSection& operator=(const ScopedSection&) = delete;

      private:
        std::atomic<int>& counter_;
        std::chrono::steady_clock::time_point start_;
    };

    /// Create a scoped timing guard for the given section index.
    [[nodiscard]] ScopedSection scope(int index) {
        return ScopedSection(sections_[index]->us);
    }

    /// Export as ProfileEntry vector for IScenePresenter.
    std::vector<IScenePresenter::ProfileEntry> entries() const {
        std::vector<IScenePresenter::ProfileEntry> result;
        result.reserve(sections_.size());
        for (const auto& s : sections_) {
            result.push_back({s->name, &s->us});
        }
        return result;
    }

  private:
    struct Section {
        const char* name;
        std::atomic<int> us{0};
    };
    std::vector<std::unique_ptr<Section>> sections_;
};
