#include "moderation/ModerationHeat.h"

#include <chrono>
#include <cmath>
#include <numbers>
#include <vector>

namespace moderation {

void ModerationHeat::configure(const HeatConfig& cfg) {
    std::lock_guard lock(mu_);
    cfg_ = cfg;
}

int64_t ModerationHeat::now_ms() const {
    if (clock_)
        return clock_();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

void ModerationHeat::decay_locked(Entry& entry, int64_t now_ms) const {
    if (entry.last_update_ms == 0) {
        entry.last_update_ms = now_ms;
        return;
    }
    const double dt = static_cast<double>(now_ms - entry.last_update_ms) / 1000.0;
    if (dt <= 0.0)
        return;

    // Exponential decay with configurable half-life.
    //   heat_new = heat_old * 2^(-dt / half_life)
    // Using exp(-dt * ln2 / half_life) for slightly better numerics.
    const double half_life = std::max(1.0, cfg_.decay_half_life_seconds);
    const double factor = std::exp(-dt * std::numbers::ln2 / half_life);
    entry.heat *= factor;
    // Hard-zero floor: exponential decay approaches but never reaches
    // zero. Without this floor a once-flagged IPID stays in LOG state
    // forever (heat ~= 1e-30), which leaves a permanent trail in the
    // gauge and delays the sweep() prune. 1e-6 is well below the
    // censor threshold (1.0) so snapping to zero can't affect action
    // decisions — it just keeps the numbers clean.
    //
    // std::abs() because trust-bank accumulation (accrue_trust) puts
    // heat on the negative side of zero. A plain `heat < 1e-6` check
    // would clobber any meaningful negative value (e.g. -5.0 < 1e-6
    // is true) on the very first decay pass, defeating trust
    // accumulation entirely.
    if (std::abs(entry.heat) < 1e-6)
        entry.heat = 0.0;
    entry.last_update_ms = now_ms;
}

double ModerationHeat::apply(const std::string& ipid, double delta) {
    if (delta < 0.0)
        delta = 0.0;

    std::lock_guard lock(mu_);
    const int64_t t = now_ms();
    auto& entry = state_[ipid];
    decay_locked(entry, t);
    // Trust-reset semantic: any accumulated trust credit (negative
    // heat from accrue_trust) is wiped BEFORE the positive delta is
    // applied. Without this, a user with -5.0 trust who trips a slur
    // filter for delta=6.0 ends up at heat=1.0 (below mute_threshold),
    // effectively absorbing 83% of the penalty. Trust is allowed to
    // accelerate the decision to SKIP the remote classifier, never
    // the decision to enforce. See apply() doc comment in the header
    // for the full rationale.
    if (entry.heat < 0.0)
        entry.heat = 0.0;
    entry.heat += delta;
    return entry.heat;
}

double ModerationHeat::accrue_trust(const std::string& ipid, double amount, double floor) {
    if (amount <= 0.0)
        return peek(ipid);
    if (floor >= 0.0)
        return peek(ipid); // floor must be negative to accrue trust toward it

    std::lock_guard lock(mu_);
    const int64_t t = now_ms();
    auto& entry = state_[ipid];
    decay_locked(entry, t);
    // Suspicion dominates trust: a user currently above zero (in the
    // LOG/CENSOR/DROP/MUTE ladder) cannot start building trust credit.
    // They have to decay naturally back to zero first. This prevents
    // a malicious user from bouncing between mild misbehavior and
    // clean traffic to game the trust-skip gate.
    if (entry.heat > 0.0)
        return entry.heat;
    entry.heat -= amount; // move toward the negative floor
    if (entry.heat < floor)
        entry.heat = floor;
    return entry.heat;
}

double ModerationHeat::peek(const std::string& ipid) {
    std::lock_guard lock(mu_);
    auto it = state_.find(ipid);
    if (it == state_.end())
        return 0.0;
    decay_locked(it->second, now_ms());
    return it->second.heat;
}

ModerationAction ModerationHeat::decide(double heat) const {
    // Threshold ladder. Higher thresholds dominate — we check from the
    // top so the strongest action wins.
    if (heat >= cfg_.perma_ban_threshold)
        return ModerationAction::PERMA_BAN;
    if (heat >= cfg_.ban_threshold)
        return ModerationAction::BAN;
    if (heat >= cfg_.kick_threshold)
        return ModerationAction::KICK;
    if (heat >= cfg_.mute_threshold)
        return ModerationAction::MUTE;
    if (heat >= cfg_.drop_threshold)
        return ModerationAction::DROP;
    if (heat >= cfg_.censor_threshold)
        return ModerationAction::CENSOR;
    if (heat > 0.0)
        return ModerationAction::LOG;
    return ModerationAction::NONE;
}

void ModerationHeat::reset(const std::string& ipid) {
    std::lock_guard lock(mu_);
    state_.erase(ipid);
}

void ModerationHeat::sweep() {
    std::lock_guard lock(mu_);
    const int64_t t = now_ms();
    const int64_t idle_ms = static_cast<int64_t>(cfg_.sweep_idle_seconds) * 1000;

    std::vector<std::string> to_erase;
    for (auto& [ipid, entry] : state_) {
        // Check idle BEFORE decaying. decay_locked() updates last_update_ms
        // to `now`, so if we decayed first, the idle check would always
        // see dt=0 and never prune anything.
        const bool idle_long_enough = (t - entry.last_update_ms) > idle_ms;
        decay_locked(entry, t);
        // std::abs() to cover the trust-bank (negative heat) case:
        // without it, a trusted user's -5.0 heat would satisfy
        // `heat < prune_below` on the very first idle sweep and lose
        // all accumulated credit. decay_locked already snaps
        // sufficiently-small magnitudes to exact zero, so this check
        // still prunes stale entries on either side of zero.
        if (std::abs(entry.heat) < cfg_.prune_below && idle_long_enough)
            to_erase.push_back(ipid);
    }
    for (auto& k : to_erase)
        state_.erase(k);
}

size_t ModerationHeat::size() const {
    std::lock_guard lock(mu_);
    return state_.size();
}

} // namespace moderation
