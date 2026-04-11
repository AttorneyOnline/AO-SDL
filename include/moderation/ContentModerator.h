/**
 * @file ContentModerator.h
 * @brief Top-level coordinator for content moderation.
 *
 * ContentModerator is the thing packet handlers actually call. It:
 *
 *   1. Runs the configured Layer 1 rules (unicode, urls) synchronously.
 *   2. (Phase 2+) Dispatches a remote classifier call.
 *   3. (Phase 3+) Runs cross-message semantic clustering.
 *   4. Sums axis scores into a heat delta and applies it to ModerationHeat.
 *   5. Picks an action from the heat ladder.
 *   6. Writes a ModerationEvent to the audit log (including sqlite).
 *
 * It is stateless from the packet handler's perspective — the handler
 * passes a message in, gets a ModerationVerdict out, and decides what
 * to do (broadcast, drop, kick). Sanctions that involve side effects
 * on the BanManager are dispatched via an ActionCallback, so this
 * class doesn't need to know about BanManager or the WS server.
 *
 * Phase 1 scope: Layers 1 + heat + audit log. Layers 2 & 3 are declared
 * in config but not wired here. Hooks in the `check()` path will be
 * filled in as follow-up commits.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"
#include "moderation/EmbeddingBackend.h"
#include "moderation/ModerationAuditLog.h"
#include "moderation/ModerationHeat.h"
#include "moderation/ModerationTypes.h"
#include "moderation/RemoteClassifier.h"
#include "moderation/SafeHintLayer.h"
#include "moderation/SemanticClusterer.h"
#include "moderation/SlurFilter.h"
#include "moderation/UnicodeClassifier.h"
#include "moderation/UrlExtractor.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

class DatabaseManager;

namespace moderation {

/// Callback invoked when the moderator wants the host application to
/// apply a sanction. The host decides how to implement it (for example,
/// BAN hits BanManager + closes the WS; MUTE is enforced at the next
/// check() call by the moderator's own mute table).
///
/// Arguments:
///   ipid    — client identifier.
///   action  — the sanction level.
///   reason  — human-readable reason to show the user or log.
using ActionCallback = std::function<void(const std::string& ipid, ModerationAction action, const std::string& reason)>;

class ContentModerator {
  public:
    ContentModerator() = default;
    ~ContentModerator() = default;

    ContentModerator(const ContentModerator&) = delete;
    ContentModerator& operator=(const ContentModerator&) = delete;

    /// Apply configuration. Safe to call at runtime — future incidents
    /// use the new config, past heat and active mutes are preserved.
    void configure(const ContentModerationConfig& cfg);

    /// Wire the audit log fan-out. Optional; if null, audit events
    /// are dropped on the floor (useful for unit tests).
    void set_audit_log(ModerationAuditLog* audit);

    /// Wire the database manager. Used for persisting moderation
    /// events and mutes. Optional; if null, moderation works entirely
    /// in memory and does not survive restart.
    void set_database(DatabaseManager* db);

    /// Wire the sanction callback. Called from check() for any action
    /// above LOG. The callback may itself take a mutex — it is NOT
    /// called with the moderator's mutex held.
    void set_action_callback(ActionCallback cb);

    /// Swap in a custom remote classifier transport (mock for tests).
    /// Must be called after configure().
    void set_remote_transport(std::unique_ptr<RemoteClassifierTransport> t);

    /// Install the embedding backend used by the semantic clustering
    /// layer. Typically called once at startup after the HF model has
    /// been fetched. Use make_embedding_backend() for the default, or
    /// provide a custom implementation for testing.
    void set_embedding_backend(std::unique_ptr<EmbeddingBackend> backend);

    /// Install the Layer 1c slur wordlist. Called from main.cpp on a
    /// background thread after the TextListFetcher completes. Takes
    /// raw newline entries — normalization and deduplication happen
    /// inside SlurFilter::load_wordlist().
    void set_slur_wordlist(const std::vector<std::string>& raw);

    /// Install the Layer 1c exception list (tokens to suppress). Same
    /// background-fetch pattern as set_slur_wordlist.
    void set_slur_exceptions(const std::vector<std::string>& raw);

    /// Install the safe-hint anchor list. Computes embeddings via the
    /// currently-installed embedding backend (must be ready) and
    /// stores them internally. Called from main.cpp on the same
    /// background thread that fetched the anchor list, AFTER the
    /// HF model fetch completes.
    ///
    /// Returns the number of anchors that successfully embedded.
    /// A zero return means the layer stays inert (embedding backend
    /// not ready, or every anchor failed to embed).
    size_t set_safe_hint_anchors(const std::vector<std::string>& raw);

    /// Decide what to do with a message. Called from the OOC and IC
    /// packet handlers before broadcast.
    ///
    /// @param ipid      client identifier
    /// @param channel   "ic" or "ooc"
    /// @param message   the raw message text (UTF-8)
    ModerationVerdict check(const std::string& ipid, std::string_view channel, std::string_view message);

    /// Information about an active mute — reason and remaining time.
    /// Populated by get_mute_info(); empty optional means not muted.
    struct MuteInfo {
        std::string reason;
        int64_t expires_at = 0; ///< Seconds since epoch; 0 = permanent (lifted manually only).
        /// Seconds until expiry, or -1 for a permanent mute. Callers
        /// should NOT render 0 as "expired" — a permanent mute has
        /// seconds_remaining == -1, not 0. A temporary mute that's
        /// exactly at its expiry instant produces seconds_remaining
        /// == 0 for the single call where that happens; by the next
        /// call is_muted/get_mute_info will report not-muted.
        int seconds_remaining = -1;
    };

    /// True if the given IPID is currently under an active mute.
    /// Packet handlers should call this BEFORE check() to enforce
    /// mutes from prior offenses without running the Layer 1 scan.
    bool is_muted(const std::string& ipid) const;

    /// Return mute metadata for reporting to the user. Returns an
    /// empty optional if the IPID is not currently muted.
    std::optional<MuteInfo> get_mute_info(const std::string& ipid) const;

    /// Manually lift a mute. Returns true if the IPID was muted.
    bool lift_mute(const std::string& ipid);

    /// Reset the heat accumulator for an IPID AND lift any active
    /// mute. Intended for the `/modheat reset` moderator command.
    /// Returns true if any state was cleared.
    bool reset_state(const std::string& ipid);

    /// Periodic housekeeping: prune decayed heat entries and expired
    /// mutes. Call every 30s from the same sweep loop as SpamDetector.
    void sweep();

    /// Read the current heat for an IPID. For REPL/admin diagnostics.
    double current_heat(const std::string& ipid);

    /// Point-in-time summary statistics over the full heat table.
    /// Used by the Prometheus collector to export gauges without
    /// blowing up cardinality with per-IPID labels.
    struct HeatStats {
        size_t tracked_ipids = 0; ///< Number of IPIDs with nonzero heat.
        size_t muted_ipids = 0;   ///< Number of currently-active mutes.
        double max_heat = 0.0;    ///< Highest current heat across all IPIDs.
        double sum_heat = 0.0;    ///< Sum of current heat (mean = sum / tracked).
        size_t above_censor = 0;  ///< Count of IPIDs >= censor_threshold.
        size_t above_drop = 0;
        size_t above_mute = 0;
    };
    HeatStats compute_heat_stats();

  private:
    /// Convert per-axis scores to a heat delta using HeatConfig weights.
    /// Takes the weight vector explicitly so the caller's cfg snapshot
    /// (captured under mu_ at check() entry) is the single source of
    /// truth for the whole call — no data race on cfg_ reads.
    static double heat_delta_from(const ModerationAxisScores& scores, const HeatConfig& h);

    /// Format a human-readable reason string from triggered axes.
    static std::string format_reason(const ModerationVerdict& v);

    /// Truncate a message to the given byte length, respecting UTF-8
    /// boundaries. Free of cfg_ so it's safe to call from any thread.
    static std::string sample(std::string_view message, int max_bytes);

    /// Atomic-ish snapshot of cfg_. Holds mu_ only long enough to copy
    /// the shared_ptr — callers get a stable view of the config for
    /// the duration of their call, and a concurrent configure() can
    /// swap in a new pointer without invalidating it.
    std::shared_ptr<const ContentModerationConfig> cfg_snapshot() const;

    /// Active config as a shared_ptr<const> under atomic swap. check()
    /// does a single atomic load at entry into a local shared_ptr and
    /// reads fields through it for the lifetime of the call; configure()
    /// builds a new ContentModerationConfig, atomically swaps the
    /// pointer, and the old config stays alive for any in-flight check()
    /// calls that already captured it.
    ///
    /// This pattern lets concurrent check() calls run entirely lock-
    /// free over cfg_ while configure() remains safe to call at
    /// runtime. Without it, every cfg_.field read from a check() on
    /// one thread would race a configure() on another.
    std::shared_ptr<const ContentModerationConfig> cfg_;
    UnicodeClassifier unicode_;
    UrlExtractor urls_;
    SlurFilter slurs_;
    RemoteClassifier remote_;
    SafeHintLayer safe_hint_;
    SemanticClusterer clusterer_;
    ModerationHeat heat_;

    ModerationAuditLog* audit_ = nullptr;
    DatabaseManager* db_ = nullptr;
    ActionCallback action_cb_;

    /// Non-owning pointer to the embedding backend that
    /// SemanticClusterer owns via unique_ptr. Cached here so the
    /// SafeHintLayer can use the same backend without a second
    /// ownership boundary. Set by set_embedding_backend() before it
    /// forwards ownership to clusterer_; cleared to nullptr if the
    /// backend is replaced with one that isn't is_ready(). Access
    /// is thread-safe because the pointer itself is only mutated on
    /// the startup path.
    EmbeddingBackend* embedding_backend_ = nullptr;

    struct ActiveMute {
        int64_t expires_at = 0; ///< Seconds since epoch, 0 = never expires.
        std::string reason;     ///< Human-readable reason from the verdict.
    };

    /// Per-IPID token bucket state for Layer 2 rate limiting. Every
    /// remote classifier call consumes one token; tokens refill at
    /// `kRemoteRefillPerSec` with a burst cap of `kRemoteBurst`.
    /// Bounds OpenAI API cost per abusing client to O(burst) regardless
    /// of how fast they send messages — Layer 1 still runs.
    struct RemoteBucket {
        double tokens = 0.0;
        int64_t last_refill_ms = 0;
    };

    /// Atomically consume one token from the remote classifier bucket
    /// for @p ipid. Returns true if the call is allowed.
    bool remote_bucket_allow(const std::string& ipid);

    /// Probabilistic skip decision for the trust-bank layer. Returns
    /// true if the current heat for @p ipid is sufficiently negative
    /// (below -cfg.api_skip_threshold) AND a uniform random draw lands
    /// under the sampling rate computed from the heat value.
    ///
    /// Sampling rate is a linear ramp: 100% API-call rate at exactly
    /// -api_skip_threshold (no skip), tapering to min_sample_rate at
    /// -max_trust (maximum skip). The floor ensures even the most
    /// trusted user has a fraction of their traffic sent to the
    /// remote classifier for ground-truth drift detection.
    ///
    /// NOT const because peek() mutates the underlying Entry (decays
    /// it in place). Guarded by heat_'s internal mutex.
    bool should_skip_for_trust_bank(const std::string& ipid, const TrustBankConfig& cfg);

    mutable std::mutex mu_;
    /// ipid -> mute state. In-memory mirror of the mutes table.
    /// Loaded on configure() from db if set.
    std::unordered_map<std::string, ActiveMute> active_mutes_;
    /// ipid -> remote classifier token bucket. Pruned by sweep() when
    /// the IPID hasn't made a call in sweep_idle_seconds.
    std::unordered_map<std::string, RemoteBucket> remote_buckets_;
};

/// Register the Prometheus metric collectors for a ContentModerator
/// instance. Idempotent: calling it twice re-registers the collector
/// so scraping continues to reflect the latest pointer. Must be called
/// after configure() (so heat thresholds are populated for bucketing).
void register_moderator_metrics(ContentModerator& cm);

} // namespace moderation
