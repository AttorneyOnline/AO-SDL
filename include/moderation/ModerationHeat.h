/**
 * @file ModerationHeat.h
 * @brief Per-IPID heat accumulator with exponential decay.
 *
 * The heat model is the hysteresis layer that sits between per-message
 * axis scores and concrete actions. It answers two questions:
 *
 *   1. "How bad has this user been recently?" — a floating-point
 *      accumulator that rises with each incident and decays between.
 *   2. "Given their current heat, what should happen next?" — a
 *      deterministic lookup into an action ladder.
 *
 * The accumulator is NOT a moving average of a fixed window; it is an
 * unbounded-in-time exponential decay. That means one very bad message
 * can push a user past the mute threshold immediately, while a drip of
 * borderline messages accumulates over many minutes.
 *
 * Thread safety: all operations take a single internal mutex, because
 * the table is small (one entry per active troublemaker) and the
 * critical sections are microseconds.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"
#include "moderation/ModerationTypes.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace moderation {

class ModerationHeat {
  public:
    ModerationHeat() = default;

    void configure(const HeatConfig& cfg);

    /// Apply @p delta heat to the counter for @p ipid, first decaying
    /// the previous value by the elapsed time. Returns the new heat.
    ///
    /// Delta should always be >= 0 — the accumulator never decreases
    /// except via decay or explicit reset().
    double apply(const std::string& ipid, double delta);

    /// Read the current heat for an IPID without adding anything.
    /// Still decays the stored value by elapsed time as a side effect.
    double peek(const std::string& ipid);

    /// Decide what action to take given a heat level. Pure function.
    /// The same heat always maps to the same action; the mutex is not
    /// needed here.
    ModerationAction decide(double heat) const;

    /// Clear the heat for a specific IPID (e.g. after mod review).
    void reset(const std::string& ipid);

    /// Prune entries whose decayed heat has fallen below
    /// HeatConfig::prune_below and that have been idle for at least
    /// HeatConfig::sweep_idle_seconds. Call periodically (~30s).
    void sweep();

    /// Number of tracked IPIDs (for metrics).
    size_t size() const;

    /// For tests: override the clock source. The function must return
    /// milliseconds since some epoch — absolute value doesn't matter.
    using Clock = int64_t (*)();
    void set_clock_for_tests(Clock clock) {
        clock_ = clock;
    }

  private:
    struct Entry {
        double heat = 0.0;
        int64_t last_update_ms = 0;
    };

    /// Decay @p entry to the current time. Caller must hold mu_.
    void decay_locked(Entry& entry, int64_t now_ms) const;

    int64_t now_ms() const;

    mutable std::mutex mu_;
    std::unordered_map<std::string, Entry> state_;
    HeatConfig cfg_;
    Clock clock_ = nullptr;
};

} // namespace moderation
