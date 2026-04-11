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
    /// Delta should always be >= 0 — apply() clamps negative deltas
    /// to zero.
    ///
    /// **Trust reset semantic**: before the positive delta is added,
    /// any currently-negative heat (accumulated via accrue_trust())
    /// is reset to zero. Rationale: a slur hit should not be absorbed
    /// by accumulated trust credit. A user with -5.0 trust who sends
    /// a slur (delta=6.0) should end up at heat=6.0 (mute threshold),
    /// not 1.0 (below any threshold). Trust accelerates the decision
    /// to SKIP the remote classifier, never the decision to act.
    double apply(const std::string& ipid, double delta);

    /// Accrue "trust" for @p ipid by subtracting @p amount from heat,
    /// clamped at -@p floor (the most negative the heat can go).
    ///
    /// Only applies when current heat <= 0. If the IPID is currently
    /// in a suspicion state (heat > 0), this is a no-op — the user
    /// has to decay naturally back to zero before they can start
    /// building trust. Decays the existing entry first like apply().
    ///
    /// Intended to be called from ContentModerator when a check()
    /// produces a NONE verdict (clean message). Used by the trust-
    /// bank layer to probabilistically skip the expensive remote
    /// classifier call for well-behaved users.
    ///
    /// Returns the new heat value (may be the same as before if the
    /// IPID is in suspicion state or the floor has been reached).
    double accrue_trust(const std::string& ipid, double amount, double floor);

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
