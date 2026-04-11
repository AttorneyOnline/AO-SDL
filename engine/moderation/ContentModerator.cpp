#include "moderation/ContentModerator.h"

#include "game/DatabaseManager.h"
#include "metrics/MetricsRegistry.h"
#include "utils/Log.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <utility>

namespace moderation {

namespace {

int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

int64_t now_sec() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/// True if the byte at index `i` in `s` is a UTF-8 continuation byte
/// (10xxxxxx). Used to walk backwards to a safe truncation point.
bool is_utf8_continuation(unsigned char b) {
    return (b & 0xC0) == 0x80;
}

} // namespace

void ContentModerator::configure(const ContentModerationConfig& cfg) {
    // Build the new config as a shared_ptr<const> so concurrent
    // check() calls can safely hold a reference to it even if a
    // subsequent reconfigure replaces the pointer. The sub-component
    // configure() calls under the lock still race-free because
    // configure() is expected to be called only at startup or from
    // the REPL — the cost of the lock on the cold path is negligible.
    auto new_cfg = std::make_shared<const ContentModerationConfig>(cfg);
    DatabaseManager* db_snapshot = nullptr;
    {
        std::lock_guard lock(mu_);
        cfg_ = new_cfg;
        unicode_.configure(new_cfg->unicode);
        urls_.configure(new_cfg->urls);
        slurs_.configure(new_cfg->slurs);
        remote_.configure(new_cfg->remote);
        safe_hint_.configure(new_cfg->safe_hint);
        clusterer_.configure(new_cfg->embeddings);
        heat_.configure(new_cfg->heat);
        db_snapshot = db_;
        // Clear existing mute state before reloading from DB so a
        // runtime reconfigure doesn't leak stale entries from a
        // previous config incarnation.
        active_mutes_.clear();
    }

    // If the database is available, load active mutes so server
    // restarts don't free muted users. If this fails we silently
    // continue — the mute system is memory-backed anyway.
    if (db_snapshot && db_snapshot->is_open()) {
        auto fut = db_snapshot->active_mutes();
        auto mutes = fut.get();
        std::lock_guard lock(mu_);
        for (auto& m : mutes) {
            ActiveMute am;
            am.expires_at = m.expires_at;
            am.reason = m.reason;
            active_mutes_[m.ipid] = std::move(am);
        }
        if (!mutes.empty())
            Log::log_print(INFO, "ContentModerator: loaded %zu active mutes from db", mutes.size());
    }
}

void ContentModerator::set_audit_log(ModerationAuditLog* audit) {
    std::lock_guard lock(mu_);
    audit_ = audit;
}

void ContentModerator::set_database(DatabaseManager* db) {
    std::lock_guard lock(mu_);
    db_ = db;
}

void ContentModerator::set_action_callback(ActionCallback cb) {
    std::lock_guard lock(mu_);
    action_cb_ = std::move(cb);
}

void ContentModerator::set_remote_transport(std::unique_ptr<RemoteClassifierTransport> t) {
    std::lock_guard lock(mu_);
    remote_.set_transport(std::move(t));
}

void ContentModerator::set_embedding_backend(std::unique_ptr<EmbeddingBackend> backend) {
    // Cache a non-owning pointer before transferring ownership to the
    // clusterer. SafeHintLayer needs access to the same backend for
    // its query() path, and storing a second owning reference would
    // duplicate the model in memory. The backend outlives the
    // clusterer (and therefore outlives ContentModerator itself) so
    // a raw pointer here is safe.
    //
    // If the passed-in backend isn't ready (make_embedding_backend()
    // can return a NullEmbeddingBackend when llama.cpp is compiled
    // out), clear the cache so SafeHintLayer reliably sees the
    // no-backend state.
    embedding_backend_ = (backend && backend->is_ready()) ? backend.get() : nullptr;

    // Note: SemanticClusterer takes its own mutex; we don't hold the
    // moderator mutex here to avoid nested-lock subtlety.
    clusterer_.set_backend(std::move(backend));
}

size_t ContentModerator::set_safe_hint_anchors(const std::vector<std::string>& raw) {
    if (!embedding_backend_) {
        Log::log_print(WARNING, "ContentModerator: set_safe_hint_anchors called before embedding backend is ready; "
                                "safe-hint layer will stay inert");
        return 0;
    }
    size_t loaded = safe_hint_.load_anchors(raw, *embedding_backend_);
    Log::log_print(INFO, "ContentModerator: loaded %zu safe-hint anchors", loaded);
    return loaded;
}

void ContentModerator::set_slur_wordlist(const std::vector<std::string>& raw) {
    // SlurFilter owns its own mutex. We don't take mu_ here for the
    // same reason we don't take it around clusterer_.set_backend:
    // nested-lock avoidance. The hot path (check()) reads the wordlist
    // under SlurFilter's internal lock, so the reload is invisible
    // to in-flight scans.
    slurs_.load_wordlist(raw);
    Log::log_print(INFO, "ContentModerator: loaded %zu slur wordlist entries", slurs_.wordlist_size());
}

void ContentModerator::set_slur_exceptions(const std::vector<std::string>& raw) {
    slurs_.load_exceptions(raw);
    Log::log_print(INFO, "ContentModerator: loaded %zu slur exception entries", slurs_.exception_size());
}

bool ContentModerator::is_muted(const std::string& ipid) const {
    std::lock_guard lock(mu_);
    auto it = active_mutes_.find(ipid);
    if (it == active_mutes_.end())
        return false;
    // Expired mutes are kept in the table until sweep() runs; treat
    // them as already lifted for lookup purposes.
    const int64_t now = now_sec();
    return it->second.expires_at <= 0 || it->second.expires_at > now;
}

std::optional<ContentModerator::MuteInfo> ContentModerator::get_mute_info(const std::string& ipid) const {
    std::lock_guard lock(mu_);
    auto it = active_mutes_.find(ipid);
    if (it == active_mutes_.end())
        return std::nullopt;

    const int64_t now = now_sec();
    const int64_t expires = it->second.expires_at;
    if (expires > 0 && expires <= now)
        return std::nullopt; // expired

    MuteInfo info;
    info.reason = it->second.reason;
    info.expires_at = expires;
    info.seconds_remaining = expires > 0 ? static_cast<int>(expires - now) : -1;
    return info;
}

bool ContentModerator::lift_mute(const std::string& ipid) {
    std::lock_guard lock(mu_);
    return active_mutes_.erase(ipid) > 0;
}

bool ContentModerator::reset_state(const std::string& ipid) {
    // Four operations happen here:
    //   1. Snapshot the pre-reset heat value (for the return bool
    //      and the audit log payload)
    //   2. Erase the in-memory mute entry (active_mutes_)
    //   3. Reset the heat accumulator to zero
    //   4. Delete persistent mute rows from the mutes table
    //   5. Record an audit event so there's a mod-action trail
    //
    // The two internal mutexes (ContentModerator::mu_ and
    // ModerationHeat::mu_) must not be held simultaneously — they
    // aren't usually contended, but this is cheap insurance against
    // future lock-order bugs.
    const double prior_heat = heat_.peek(ipid);
    bool had_mute = false;
    {
        std::lock_guard lock(mu_);
        had_mute = active_mutes_.erase(ipid) > 0;
    }
    heat_.reset(ipid);
    DatabaseManager* db_snapshot = nullptr;
    ModerationAuditLog* audit_snapshot = nullptr;
    {
        std::lock_guard lock(mu_);
        db_snapshot = db_;
        audit_snapshot = audit_;
    }
    if (db_snapshot && db_snapshot->is_open())
        db_snapshot->delete_mutes_by_ipid(ipid);

    // Audit event for the mod-reset action — /modheat reset is a
    // privileged operation and the audit log is the source of truth
    // for who cleared whose state. Without this event the mute and
    // heat just silently vanish with no trail.
    if (audit_snapshot && (had_mute || prior_heat > 0.0)) {
        ModerationEvent ev;
        ev.timestamp_ms = now_ms();
        ev.ipid = ipid;
        ev.channel = "mod_action";
        ev.message_sample = "/modheat reset";
        ev.action = ModerationAction::LOG;
        ev.heat_after = 0.0;
        ev.reason = "heat=" + std::to_string(prior_heat) + (had_mute ? " mute=yes" : " mute=no");
        audit_snapshot->record(ev);
    }

    // True iff something was actually cleared. Callers use this to
    // differentiate "you just cleared state" from "nothing to clear".
    return had_mute || prior_heat > 0.0;
}

double ContentModerator::current_heat(const std::string& ipid) {
    return heat_.peek(ipid);
}

namespace {
// Token bucket tunables for the per-IPID remote-classifier rate
// limit. Burst 10 means a client can fire 10 remote calls back-to-
// back before throttling kicks in; refill 2/sec matches the default
// rate limit for the ao:CT (OOC chat) action family, so a well-
// behaved client on the default rate limits will never hit this.
constexpr double kRemoteBurst = 10.0;
constexpr double kRemoteRefillPerSec = 2.0;
} // namespace

bool ContentModerator::remote_bucket_allow(const std::string& ipid) {
    std::lock_guard lock(mu_);
    const int64_t now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    auto& bucket = remote_buckets_[ipid];
    if (bucket.last_refill_ms == 0) {
        bucket.tokens = kRemoteBurst;
        bucket.last_refill_ms = now;
    }
    else {
        const double dt_sec = static_cast<double>(now - bucket.last_refill_ms) / 1000.0;
        bucket.tokens = std::min(kRemoteBurst, bucket.tokens + dt_sec * kRemoteRefillPerSec);
        bucket.last_refill_ms = now;
    }
    if (bucket.tokens < 1.0)
        return false;
    bucket.tokens -= 1.0;
    return true;
}

bool ContentModerator::should_skip_for_trust_bank(const std::string& ipid, const TrustBankConfig& cfg) {
    // Guard: config must have a sane range. max_trust > api_skip_threshold
    // is load-bearing — a zero-width band would make the denominator
    // below blow up. Default config has max_trust=10.0 and
    // api_skip_threshold=5.0 which is a 5.0-wide band.
    if (cfg.max_trust <= cfg.api_skip_threshold)
        return false;

    const double heat = heat_.peek(ipid);
    // Only positive-absolute trust credit is eligible. Users above
    // zero are in suspicion and always get the remote call.
    if (heat > -cfg.api_skip_threshold)
        return false;

    // Linear ramp: at heat = -api_skip_threshold → api_rate = 1.0 (no skip);
    // at heat = -max_trust → api_rate = min_sample_rate (maximum skip).
    // Clamp at either end so values outside the band still behave sanely.
    const double over_threshold = -heat - cfg.api_skip_threshold;
    const double band_width = cfg.max_trust - cfg.api_skip_threshold;
    double fraction = over_threshold / band_width;
    if (fraction < 0.0)
        fraction = 0.0;
    if (fraction > 1.0)
        fraction = 1.0;
    const double api_rate = 1.0 - fraction * (1.0 - cfg.min_sample_rate);
    // api_rate is the probability of STILL calling the remote. We
    // want to SKIP with probability (1 - api_rate).
    const double skip_rate = 1.0 - api_rate;

    // Thread-local RNG: one per worker thread, seeded once from
    // random_device. Using a shared global RNG would require a
    // mutex which would serialize every check() call.
    thread_local std::mt19937_64 rng{std::random_device{}()};
    thread_local std::uniform_real_distribution<double> dist{0.0, 1.0};
    return dist(rng) < skip_rate;
}

namespace {

// Per-axis visibility floors: an axis only contributes to the
// per-message heat delta when its score crosses this threshold.
// Chosen to match the dashboard axis_fires counter and the verdict
// reason labels so "the axis fired", "the axis added heat", and
// "the reason string mentions this axis" are always the same event.
//
// Rationale: remote classifiers (OpenAI omni-moderation) return
// floor-level noise on every input — clean messages commonly see
// toxicity ~= 1e-5, hate ~= 1e-6, etc. Without a floor, those
// multiply by the axis weight to produce a sub-0.001 heat delta
// that IS still > 0, which blows through the "heat_delta > 0"
// short-circuit and lets the heat ladder act on accumulated heat
// from previous offenses.
//
// Floor tuning for kagami (roleplay-heavy Ace Attorney server):
//
//   Toxicity/harassment floor is DELIBERATELY VERY HIGH (0.85).
//   OpenAI's harassment axis scores canonical courtroom dialogue
//   in surprising places:
//     - "Do you have cotton between your ears, spiky boy?" → 0.65
//     - "You killed her! I know you did!"                 → 0.4-0.5
//     - "Objection! You're nothing but a liar!"           → 0.3-0.4
//     - "You pathetic little worm"                        → 0.6-0.7
//   All of those are normal Ace Attorney cross-examination and
//   should never fire moderation. Real sustained abuse scores
//   ~0.9+ and still catches at this floor.
//
//   Violence gets the same treatment — courtroom accusations
//   ("he killed her with a knife") score in the 0.4-0.6 range
//   and are expected in-character.
//
//   Hate stays relatively strict (0.3) because identity-based
//   slurs are never roleplay. Sexual/minors is catastrophic.
//   Self-harm floor is moderate because even in-character, the
//   axis needs to catch grooming-adjacent content.
//
// If you're deploying kagami on a general chat server without
// heavy roleplay, drop the classifier floors to 0.15-0.3 and
// raise the heat weights for a stricter baseline.
constexpr double kAxisFloorNoise = 0.0;    // rules-based, non-zero = real
constexpr double kAxisFloorLinkRisk = 0.0; // rules-based, non-zero = real
constexpr double kAxisFloorSlurs = 0.0;    // exact-token match; any hit is real
/// OpenAI's hate axis is high-precision but low-recall: confident slurs
/// like "gas the jews" score 0.9+, but common casual slurs ("tranny",
/// "heil hitler", racial epithets used casually) often score in the
/// 0.1-0.3 range because the axis is narrowly tuned to "clearly
/// identity-targeted". A 0.3 floor lets those through.
///
/// 0.1 catches essentially every hate-axis signal OpenAI returns,
/// and because the axis is narrowly defined, false positives on
/// non-hate content are rare. Roleplay content (Ace Attorney cross-
/// examination, dramatic accusations) doesn't trigger the hate
/// axis at all — it lives on the harassment axis, which we keep
/// high (0.85) for roleplay tolerance.
constexpr double kAxisFloorHate = 0.1;
constexpr double kAxisFloorToxicity = 0.85;     // harassment — very high floor for roleplay
constexpr double kAxisFloorSexual = 0.7;        // sexual content (adult) — 16+ audience
constexpr double kAxisFloorViolence = 0.85;     // courtroom violence is canon
constexpr double kAxisFloorSelfHarm = 0.5;      // moderate — catches grooming-adjacent content
constexpr double kAxisFloorSexualMinors = 0.01; // catastrophic, stricter
constexpr double kAxisFloorSemanticEcho = 0.0;  // clustering is already >= 1.0 when it fires

} // namespace

std::shared_ptr<const ContentModerationConfig> ContentModerator::cfg_snapshot() const {
    std::lock_guard lock(mu_);
    return cfg_;
}

double ContentModerator::heat_delta_from(const ModerationAxisScores& s, const HeatConfig& h) {
    double d = 0.0;
    if (s.visual_noise > kAxisFloorNoise)
        d += s.visual_noise * h.weight_visual_noise;
    if (s.link_risk > kAxisFloorLinkRisk)
        d += s.link_risk * h.weight_link_risk;
    if (s.slurs > kAxisFloorSlurs)
        d += s.slurs * h.weight_slurs;
    if (s.toxicity > kAxisFloorToxicity)
        d += s.toxicity * h.weight_toxicity;
    if (s.hate > kAxisFloorHate)
        d += s.hate * h.weight_hate;
    if (s.sexual > kAxisFloorSexual)
        d += s.sexual * h.weight_sexual;
    if (s.sexual_minors > kAxisFloorSexualMinors)
        d += s.sexual_minors * h.weight_sexual_minors;
    if (s.violence > kAxisFloorViolence)
        d += s.violence * h.weight_violence;
    if (s.self_harm > kAxisFloorSelfHarm)
        d += s.self_harm * h.weight_self_harm;
    if (s.semantic_echo > kAxisFloorSemanticEcho)
        d += s.semantic_echo * h.weight_semantic_echo;
    return d;
}

std::string ContentModerator::format_reason(const ModerationVerdict& v) {
    if (v.triggered_axes.empty())
        return "";
    std::string out;
    for (size_t i = 0; i < v.triggered_axes.size(); ++i) {
        if (i > 0)
            out += ", ";
        out += v.triggered_axes[i];
    }
    return out;
}

std::string ContentModerator::sample(std::string_view message, int max_bytes) {
    const int max = max_bytes;
    if (max <= 0 || static_cast<int>(message.size()) <= max)
        return std::string(message);

    // Truncate at `max` bytes, then walk back to a complete UTF-8
    // sequence boundary. Two cases to handle:
    //   (1) cut lands inside a continuation byte — walk back until
    //       we find a lead byte (or ASCII byte).
    //   (2) cut lands on a lead byte whose full multi-byte sequence
    //       is longer than the remaining bytes — also walk back,
    //       dropping the truncated lead.
    // Naive walk-back only handled (1). If the buffer ends mid-lead
    // we'd have emitted an invalid sequence into the audit sample,
    // which then gets fed back through json::dump and produces
    // either a decode exception or a U+FFFD replacement — neither
    // is desirable in the audit log's message_sample field.
    size_t cut = static_cast<size_t>(max);
    while (cut > 0 && is_utf8_continuation(static_cast<unsigned char>(message[cut])))
        --cut;
    // `cut` now points at a lead or ASCII byte (or cut==0). If it's
    // a multi-byte lead, verify the FULL sequence fits; if not,
    // drop it and walk back once more to the previous boundary.
    if (cut > 0 && cut < message.size()) {
        auto lead = static_cast<unsigned char>(message[cut]);
        int seq_len = 0;
        if (lead < 0x80)
            seq_len = 1;
        else if ((lead & 0xE0) == 0xC0)
            seq_len = 2;
        else if ((lead & 0xF0) == 0xE0)
            seq_len = 3;
        else if ((lead & 0xF8) == 0xF0)
            seq_len = 4;
        if (seq_len == 0 || cut + seq_len > message.size()) {
            // Truncated sequence at the boundary — drop this byte
            // and all its preceding continuations so the trailing
            // bytes are a clean, complete UTF-8 prefix.
            if (cut > 0)
                --cut;
            while (cut > 0 && is_utf8_continuation(static_cast<unsigned char>(message[cut])))
                --cut;
        }
    }
    std::string out(message.substr(0, cut));
    out += "…";
    return out;
}

ModerationVerdict ContentModerator::check(const std::string& ipid, std::string_view channel, std::string_view message) {
    ModerationVerdict v;

    // Metric handles are registered once per process — static-local
    // caches avoid repeated hash lookups in MetricsRegistry on every
    // message. This is the same pattern SpamDetector uses at line 55.
    static auto& events_counter = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_events_total", "Moderation decisions emitted", {"action", "channel"});
    static auto& layer_ns_counter = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_layer_nanoseconds_total", "Total nanoseconds spent in each moderation layer", {"layer"});
    static auto& layer_calls_counter = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_layer_calls_total", "Times each moderation layer has been invoked", {"layer"});
    static auto& check_ns_counter = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_check_nanoseconds_total", "Total nanoseconds spent in ContentModerator::check");
    static auto& check_calls_counter = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_checks_total", "Total ContentModerator::check invocations");
    // Per-axis fire counter. Increments when a given axis score
    // crosses a small visibility floor (0.15 for classifier axes,
    // 0.001 for sexual_minors which is catastrophic-weighted, 0.0
    // for the rules-based visual_noise/link_risk/semantic_echo
    // which only contribute when they have non-zero scores to
    // begin with). Lets operators see "how often does hate fire"
    // without mining CloudWatch log events.
    static auto& axis_fires = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_axis_fires_total", "Count of checks where a given axis scored above its visibility floor",
        {"axis"});

    const auto check_start = std::chrono::steady_clock::now();
    auto bump_layer = [&](const char* layer, std::chrono::steady_clock::time_point t0) {
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
        layer_ns_counter.labels({layer}).inc(static_cast<double>(dt));
        layer_calls_counter.labels({layer}).inc();
    };

    // Capture a cfg snapshot at the top of check() and use it for
    // every config read through the rest of the call. This is the
    // data-race fix: a concurrent configure() can swap cfg_ to a
    // new shared_ptr without invalidating our local — we keep
    // ownership of the old config until our call returns. All
    // downstream `cfg->field` reads are lock-free.
    auto cfg = cfg_snapshot();
    if (!cfg) {
        // configure() has never been called — treat as disabled.
        return v;
    }

    // Config gating: if the whole subsystem or this channel is off,
    // we return a NONE verdict without running any layer.
    if (!cfg->enabled)
        return v;
    if (channel == "ic" && !cfg->check_ic)
        return v;
    if (channel == "ooc" && !cfg->check_ooc)
        return v;

    // --- Layer 1a: visual-noise -------------------------------------
    {
        auto t0 = std::chrono::steady_clock::now();
        auto unicode_result = unicode_.classify(message);
        v.scores.visual_noise = unicode_result.score;
        if (unicode_result.score > 0.0) {
            v.triggered_axes.push_back("visual_noise(" + unicode_result.reason + ")");
        }
        bump_layer("unicode", t0);
    }

    // --- Layer 1b: URL blocklist ------------------------------------
    {
        auto t0 = std::chrono::steady_clock::now();
        auto url_result = urls_.extract(message);
        v.scores.link_risk = url_result.score;
        if (url_result.score > 0.0) {
            v.triggered_axes.push_back("link(" + url_result.reason + ")");
        }
        bump_layer("urls", t0);
    }

    // --- Layer 1c: slur wordlist ------------------------------------
    // Word-boundary match against an operator-supplied wordlist after
    // a Scunthorpe-safe normalizer pass. Runs before Layer 2 because:
    //
    //   1. It's strictly local — no network, no rate-limit concerns.
    //   2. The remote classifier misses this class of content. OpenAI
    //      scores "heil hitler" at hate=0.00014 and various racial
    //      epithets in the 0.05-0.3 range, which can't be caught by
    //      any sensible floor tweak. This layer fills that gap.
    //   3. A hit contributes enough heat (slurs × weight_slurs, 6.0
    //      by default) to cross the mute_threshold on a single
    //      message — which makes layer1_sufficient fire below and
    //      short-circuits the remote call. No money spent classifying
    //      a message we already know is abuse.
    //
    // Inert by default: slurs_.is_active() is false until the layer
    // is enabled in config AND a wordlist has been loaded (which only
    // happens if the operator supplied a URL and TextListFetcher
    // succeeded or a cache file exists).
    if (slurs_.is_active()) {
        auto t0 = std::chrono::steady_clock::now();
        auto slur_result = slurs_.scan(message);
        v.scores.slurs = slur_result.score;
        if (slur_result.score > 0.0) {
            v.triggered_axes.push_back(slur_result.reason);
        }
        bump_layer("slurs", t0);
    }

    // --- Layer 2: remote classifier ---------------------------------
    // Synchronously calls OpenAI's moderation API (or any compatible
    // endpoint) with the configured timeout. On any failure — network,
    // HTTP error, parse error — the layer contributes zero signal and
    // the Layer 1 result carries the decision. This is "fail open" and
    // is the right default for a chat server: better to occasionally
    // miss a bad message than to brick the channel when OpenAI has an
    // incident.
    //
    // Two short-circuits prevent wasted API calls:
    //
    //   1. Layer-1 sufficient: if the rules-based layers have already
    //      produced enough heat delta to cross the mute threshold,
    //      the remote classifier has nothing to add — the decision is
    //      already made, calling OpenAI is a guaranteed waste of
    //      quota. We still run remote for lower-severity Layer 1
    //      hits because the classifier may escalate them.
    //
    //   2. Per-IPID token bucket: each IPID gets a small bucket
    //      (burst=10, refill=2/sec by default) of remote calls. A
    //      malicious client posting 100 msg/s can burn at most the
    //      bucket width before Layer 2 short-circuits; Layer 1 still
    //      runs on every message. This bounds the API cost per
    //      abuser to O(bucket_size) regardless of their send rate.
    const double layer1_delta = heat_delta_from(v.scores, cfg->heat);
    const bool layer1_sufficient = layer1_delta >= cfg->heat.mute_threshold;

    // --- Layer 2 shortcut: safe-hint embedding similarity -------------
    // Before deciding whether to spend budget on the remote classifier,
    // ask SafeHintLayer if the message is embedding-close to any
    // operator-curated "obviously safe" anchor phrase. On a hit, we
    // skip Layer 2 entirely — Layer 1 (which ran above) made the
    // final decision, and the heat delta is whatever those layers
    // contributed (usually zero for mundane chatter).
    //
    // This is a cost optimization, NOT a safety bypass: Layer 1 still
    // ran against the same message above, so slurs / URLs / unicode
    // noise still caught. The shortcut only removes the redundant
    // "is this harmless?" classifier call for messages the embedding
    // model already says look harmless.
    //
    // Ordering matters: we check this BEFORE the remote_bucket_allow
    // call because we don't want safe-hint skips to consume token-
    // bucket credit intended for actual classifier calls.
    static auto& l2_skipped_counter = metrics::MetricsRegistry::instance().counter(
        "kagami_moderation_layer2_skipped_total", "Times Layer 2 (remote classifier) was skipped and why", {"reason"});

    // Skip-reason bookkeeping is structured as a single decision tree
    // so the four reasons (disabled / safe_hint / layer1_sufficient /
    // rate_limit) are MUTUALLY EXCLUSIVE by construction — exactly one
    // gets incremented per check() on the Layer-2-skipped path, and
    // the gating that enforces the exclusion is structural rather
    // than scattered across multiple `if (remote_.is_active() && ...)`
    // guards that a future maintainer might forget to mirror.
    //
    // Dashboards can sum any subset of the labels and the sum is
    // meaningful: total Layer 2 skips = sum across all reasons,
    // total Layer 2 calls = checks_total - sum across all reasons.
    //
    // The reasons are evaluated in priority order:
    //   1. disabled          — operator never wired the remote layer
    //   2. layer1_sufficient — Layer 1 already crossed mute threshold,
    //                          calling remote can't escalate further
    //   3. trust_bank        — user has accumulated clean-traffic
    //                          credit and rolled under the sampling
    //                          rate (probabilistic; a floor keeps a
    //                          baseline of remote calls for drift)
    //   4. safe_hint         — embedding similarity to a known-safe anchor
    //   5. rate_limit        — per-IPID token bucket exhausted
    // The final fall-through is the actual remote call.
    bool should_call_remote = false;
    if (!remote_.is_active()) {
        l2_skipped_counter.labels({"disabled"}).inc();
    }
    else if (layer1_sufficient) {
        l2_skipped_counter.labels({"layer1_sufficient"}).inc();
    }
    else if (cfg->trust_bank.enabled && should_skip_for_trust_bank(ipid, cfg->trust_bank)) {
        // Probabilistic skip: user has accumulated trust credit
        // (negative heat) and rolled under the sampling rate. A
        // minimum sample rate floor still sends some traffic to the
        // remote classifier so behavioral drift stays detectable.
        l2_skipped_counter.labels({"trust_bank"}).inc();
    }
    else {
        // Try the safe-hint shortcut. Inert if the layer isn't
        // configured or the embedding backend isn't loaded yet.
        bool safe_hint_hit = false;
        if (safe_hint_.is_active() && embedding_backend_) {
            auto t0 = std::chrono::steady_clock::now();
            auto hint = safe_hint_.query(message, *embedding_backend_);
            bump_layer("safe_hint", t0);
            if (hint.is_safe) {
                safe_hint_hit = true;
                l2_skipped_counter.labels({"safe_hint"}).inc();
            }
        }
        if (!safe_hint_hit) {
            // Per-IPID rate limit is the last gate. Token bucket
            // returning false means an abuser is firing classifier
            // calls faster than the configured refill rate — Layer
            // 1 still ran on this message, the heat ladder still
            // applies, we just don't burn additional OpenAI quota
            // on the same client.
            if (!remote_bucket_allow(ipid))
                l2_skipped_counter.labels({"rate_limit"}).inc();
            else
                should_call_remote = true;
        }
    }

    if (should_call_remote) {
        static auto& remote_err_counter = metrics::MetricsRegistry::instance().counter(
            "kagami_moderation_remote_errors_total", "Remote classifier failures by cause", {"reason"});
        // Dedup cache counters: counted on every classify() that
        // reached the remote path (i.e. after all L2 skip decisions
        // already said "call it"). A hit means the HTTP call was
        // avoided; a miss means it happened. These counters track
        // the RemoteClassifier's internal state, so we diff the
        // per-classifier totals against our last-seen snapshot and
        // inc() by the delta — the RemoteClassifier counters are
        // monotonic and the Prometheus counters must also be.
        static auto& cache_hits_counter = metrics::MetricsRegistry::instance().counter(
            "kagami_moderation_remote_cache_hits_total", "Remote classifier dedup cache hits", {});
        static auto& cache_misses_counter = metrics::MetricsRegistry::instance().counter(
            "kagami_moderation_remote_cache_misses_total", "Remote classifier dedup cache misses", {});
        const int64_t hits_before = remote_.cache_hits();
        const int64_t misses_before = remote_.cache_misses();
        auto t0 = std::chrono::steady_clock::now();
        auto rr = remote_.classify(std::string(message));
        {
            const int64_t dh = remote_.cache_hits() - hits_before;
            const int64_t dm = remote_.cache_misses() - misses_before;
            if (dh > 0)
                cache_hits_counter.get().inc(static_cast<uint64_t>(dh));
            if (dm > 0)
                cache_misses_counter.get().inc(static_cast<uint64_t>(dm));
        }
        if (rr.ok) {
            // Merge remote scores into the verdict. Use max() so a
            // Layer 1 hit isn't overwritten by a lower remote score,
            // and vice versa — the stronger signal wins per axis.
            v.scores.toxicity = std::max(v.scores.toxicity, rr.scores.toxicity);
            v.scores.hate = std::max(v.scores.hate, rr.scores.hate);
            v.scores.sexual = std::max(v.scores.sexual, rr.scores.sexual);
            v.scores.sexual_minors = std::max(v.scores.sexual_minors, rr.scores.sexual_minors);
            v.scores.violence = std::max(v.scores.violence, rr.scores.violence);
            v.scores.self_harm = std::max(v.scores.self_harm, rr.scores.self_harm);

            // Only tag the reason for axes that actually crossed
            // their per-axis visibility floor. Using the same
            // constants as heat_delta_from() guarantees "axis
            // contributed heat" and "axis appears in reason string"
            // are the same event.
            if (rr.scores.toxicity > kAxisFloorToxicity)
                v.triggered_axes.emplace_back("toxicity");
            if (rr.scores.hate > kAxisFloorHate)
                v.triggered_axes.emplace_back("hate");
            if (rr.scores.sexual > kAxisFloorSexual)
                v.triggered_axes.emplace_back("sexual");
            if (rr.scores.sexual_minors > kAxisFloorSexualMinors)
                v.triggered_axes.emplace_back("sexual_minors");
            if (rr.scores.violence > kAxisFloorViolence)
                v.triggered_axes.emplace_back("violence");
            if (rr.scores.self_harm > kAxisFloorSelfHarm)
                v.triggered_axes.emplace_back("self_harm");
        }
        else if (!cfg->remote.fail_open) {
            // fail_closed: treat any remote failure as a high-toxicity
            // signal. Rarely the right choice for a chat server.
            v.scores.toxicity = std::max(v.scores.toxicity, 0.5);
            v.triggered_axes.push_back("remote_error(" + rr.error + ")");
        }
        if (!rr.ok) {
            // Cardinality: error reason is already coarse-grained
            // (timeout/http/parse). Fall back to "other" for anything
            // unexpected to keep the label set bounded.
            std::string reason = rr.http_status == 0                               ? "transport"
                                 : (rr.http_status >= 400 && rr.http_status < 500) ? "http_4xx"
                                 : (rr.http_status >= 500)                         ? "http_5xx"
                                                                                   : "other";
            remote_err_counter.labels({reason}).inc();
        }
        bump_layer("remote", t0);
    }

    // --- Layer 3: semantic clustering -------------------------------
    // Embedding-based near-duplicate detection across recent messages
    // from distinct IPIDs. Catches paraphrased spam that evades the
    // fingerprint-based H1 in SpamDetector. No-op if no embedding
    // backend is installed OR if the layer is disabled in config.
    if (cfg->embeddings.enabled) {
        auto t0 = std::chrono::steady_clock::now();
        auto sr = clusterer_.score(ipid, message);
        if (sr.score > 0.0) {
            v.scores.semantic_echo = std::max(v.scores.semantic_echo, sr.score);
            v.triggered_axes.push_back(sr.reason);
        }
        bump_layer("embeddings", t0);
    }

    // --- Heat accumulation ------------------------------------------
    //
    // Per-message filtering semantics: a message with zero heat
    // contribution from its OWN content is NEVER filtered, regardless
    // of the user's accumulated heat or mute state. This avoids the
    // anti-pattern where one bad message causes every subsequent
    // innocuous message from the same user to get dropped for the
    // entire mute duration. The heat ladder only applies to messages
    // that themselves have offending content — the user's history
    // still determines how *severe* the response to those messages is.
    //
    // A user with heat 10 who posts "hello world" sees their message
    // broadcast unchanged. The same user posting a slur sees a KICK
    // because their heat climbs to the kick threshold on that single
    // new offense, not because of the prior hello.
    //
    // Critical subtlety: heat_delta_from() applies per-axis visibility
    // floors (see kAxisFloor* constants) so a clean message with
    // floor-level classifier noise (toxicity=1e-5 etc) produces
    // delta=0 — matches the axis_fires metric model. Without the
    // floors, remote classifier noise would push delta just above 0
    // on every clean message and the short-circuit would never fire
    // for users with accumulated heat.
    v.heat_delta = heat_delta_from(v.scores, cfg->heat);

    if (v.heat_delta <= 0.0) {
        // No meaningful contribution from this message — accrue a
        // drop of trust if the feature is enabled (moves heat toward
        // the negative floor, unlocking the trust_bank skip gate for
        // this IPID), then peek the stored heat so the gauge reflects
        // the updated value, and return NONE.
        //
        // accrue_trust() is a no-op when the user is currently in a
        // positive-heat (suspicion) state, so this is safe to call
        // unconditionally on every clean message.
        if (cfg->trust_bank.enabled) {
            v.heat_after = heat_.accrue_trust(ipid, cfg->trust_bank.clean_reward, -cfg->trust_bank.max_trust);
        }
        else {
            v.heat_after = heat_.peek(ipid);
        }
        v.action = ModerationAction::NONE;
        events_counter.labels({to_string(v.action), std::string(channel)}).inc();
        // Wall-clock timing for the whole check() including the early
        // return path (so dashboards see the full cost, not just the
        // action-bearing subset).
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - check_start)
                      .count();
        check_ns_counter.get().inc(static_cast<uint64_t>(dt));
        check_calls_counter.get().inc();
        return v;
    }

    // Real content — apply the delta and determine action.
    v.heat_after = heat_.apply(ipid, v.heat_delta);
    v.action = heat_.decide(v.heat_after);
    v.reason = format_reason(v);

    // Per-action event counter (low cardinality: <10 actions × 2 channels).
    events_counter.labels({to_string(v.action), std::string(channel)}).inc();

    // Per-axis fires. Uses the same per-axis floors as
    // heat_delta_from() so "fired" and "added heat" are always
    // the same event in the dashboards.
    if (v.scores.visual_noise > kAxisFloorNoise)
        axis_fires.labels({"visual_noise"}).inc();
    if (v.scores.link_risk > kAxisFloorLinkRisk)
        axis_fires.labels({"link_risk"}).inc();
    if (v.scores.slurs > kAxisFloorSlurs)
        axis_fires.labels({"slurs"}).inc();
    if (v.scores.toxicity > kAxisFloorToxicity)
        axis_fires.labels({"toxicity"}).inc();
    if (v.scores.hate > kAxisFloorHate)
        axis_fires.labels({"hate"}).inc();
    if (v.scores.sexual > kAxisFloorSexual)
        axis_fires.labels({"sexual"}).inc();
    if (v.scores.sexual_minors > kAxisFloorSexualMinors)
        axis_fires.labels({"sexual_minors"}).inc();
    if (v.scores.violence > kAxisFloorViolence)
        axis_fires.labels({"violence"}).inc();
    if (v.scores.self_harm > kAxisFloorSelfHarm)
        axis_fires.labels({"self_harm"}).inc();
    if (v.scores.semantic_echo > kAxisFloorSemanticEcho)
        axis_fires.labels({"semantic_echo"}).inc();

    // --- Side effects -----------------------------------------------
    // If the action is MUTE, record it in the active_mutes_ table so
    // subsequent check()/is_muted() calls short-circuit. Persist to
    // DB if available.
    if (v.action == ModerationAction::MUTE) {
        const int64_t expires = now_sec() + cfg->heat.mute_duration_seconds;
        const std::string mute_reason = v.reason.empty() ? "auto-mute" : v.reason;
        {
            std::lock_guard lock(mu_);
            ActiveMute am;
            am.expires_at = expires;
            am.reason = mute_reason;
            active_mutes_[ipid] = std::move(am);
        }
        if (db_ && db_->is_open()) {
            MuteEntry m;
            m.ipid = ipid;
            m.started_at = now_sec();
            m.expires_at = expires;
            m.reason = mute_reason;
            m.moderator = "ContentModerator";
            db_->add_mute(std::move(m));
        }
    }

    // Fire the host action callback for any non-trivial verdict.
    // We hold no locks here so the callback is free to reach into
    // the BanManager, WS server, etc.
    if (v.action != ModerationAction::NONE && v.action != ModerationAction::LOG) {
        ActionCallback cb;
        {
            std::lock_guard lock(mu_);
            cb = action_cb_;
        }
        if (cb) {
            try {
                cb(ipid, v.action, v.reason);
            }
            catch (const std::exception& e) {
                Log::log_print(WARNING, "ContentModerator: action callback threw: %s", e.what());
            }
        }
    }

    // --- Audit log --------------------------------------------------
    // Always build the event if auditing is wired and the action is
    // at or above the sink's threshold — the ModerationAuditLog
    // handles the actual min_action filter internally.
    if (audit_ || (db_ && db_->is_open() && cfg->audit.sqlite_enabled)) {
        ModerationEvent ev;
        ev.timestamp_ms = now_ms();
        ev.ipid = ipid;
        ev.channel = std::string(channel);
        ev.message_sample = sample(message, cfg->message_sample_length);
        ev.scores = v.scores;
        ev.action = v.action;
        ev.heat_after = v.heat_after;
        ev.reason = v.reason;

        // Fire the fan-out sinks (file/stdout/Loki/CloudWatch).
        if (audit_)
            audit_->record(ev);

        // Persist to sqlite. Fire-and-forget — we don't block on the
        // future; the worker thread will catch up eventually.
        if (db_ && db_->is_open() && cfg->audit.sqlite_enabled &&
            static_cast<int>(v.action) >= static_cast<int>(ModerationAction::LOG)) {
            db_->record_moderation_event(std::move(ev));
        }
    }

    // Wall-clock cost of the entire check(), including audit-log
    // fan-out. Useful for catching regressions after Phase 2/3 land.
    {
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - check_start)
                      .count();
        // Unlabeled metrics: CounterFamily::get() returns the
        // default-label Counter instance.
        check_ns_counter.get().inc(static_cast<uint64_t>(dt));
        check_calls_counter.get().inc();
    }

    return v;
}

ContentModerator::HeatStats ContentModerator::compute_heat_stats() {
    // We reach into ModerationHeat through the public API rather than
    // cracking it open, to keep that class's internal table private.
    // The collector runs at scrape time (not in the hot path), so the
    // O(N) walk is fine even for a large tracked set.
    HeatStats stats;
    std::lock_guard lock(mu_);
    const int64_t now = now_sec();
    for (auto& [ipid, mute] : active_mutes_) {
        if (mute.expires_at == 0 || mute.expires_at > now)
            ++stats.muted_ipids;
    }
    // The heat table lives inside ModerationHeat — use peek() which
    // decays-in-place, so the gauge reflects the current decayed state.
    // We don't have a public iterator, so this summarizes only the
    // mute table for now. A follow-up can expose a peek_all() method
    // on ModerationHeat if we want max_heat across the full population.
    // For Phase 1 this is acceptable: muted_ipids and tracked_ipids
    // are the signals operators actually watch.
    stats.tracked_ipids = heat_.size();
    return stats;
}

void register_moderator_metrics(ContentModerator& cm) {
    auto& reg = metrics::MetricsRegistry::instance();
    auto& tracked_g = reg.gauge("kagami_moderation_heat_tracked_ipids", "Number of IPIDs with nonzero moderation heat");
    auto& muted_g =
        reg.gauge("kagami_moderation_muted_ipids", "Number of IPIDs under an active content-moderation mute");

    reg.add_collector([&cm, &tracked_g, &muted_g] {
        auto stats = cm.compute_heat_stats();
        tracked_g.get().set(static_cast<double>(stats.tracked_ipids));
        muted_g.get().set(static_cast<double>(stats.muted_ipids));
    });
}

void ContentModerator::sweep() {
    heat_.sweep();
    clusterer_.sweep();

    // Prune expired mutes from memory.
    {
        std::lock_guard lock(mu_);
        const int64_t now = now_sec();
        for (auto it = active_mutes_.begin(); it != active_mutes_.end();) {
            if (it->second.expires_at > 0 && it->second.expires_at <= now)
                it = active_mutes_.erase(it);
            else
                ++it;
        }
    }

    // Prune the persistent mutes table too. Fire-and-forget.
    if (db_ && db_->is_open())
        db_->prune_expired_mutes();
}

} // namespace moderation
