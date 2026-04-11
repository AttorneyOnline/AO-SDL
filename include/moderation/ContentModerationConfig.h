/**
 * @file ContentModerationConfig.h
 * @brief Config structs for the content moderation subsystem.
 *
 * Everything here is opt-in: the top-level `enabled` flag must be true
 * AND each individual layer has its own `enabled` flag. All defaults
 * produce zero behavior change from pre-moderation kagami so config
 * migrations are safe.
 *
 * The remote classifier layer takes an API key; the local embeddings
 * layer takes a HuggingFace model id. Neither ships any model data
 * with the binary — deploys that want the embeddings layer pull the
 * model at boot time.
 */
#pragma once

#include <string>
#include <vector>

namespace moderation {

/// Weights and thresholds for the per-IPID heat accumulator.
///
/// The heat model is simple: each message adds a weighted sum of its
/// axis scores to a per-IPID counter; the counter decays exponentially
/// between messages. The counter crossing each threshold triggers the
/// corresponding action.
///
/// Defaults are tuned for a 16+ audience: crude profanity is not an
/// axis here (it is not scored at all), and toxicity/hate have to hit
/// together a few times before anything sticks beyond a censor.
struct HeatConfig {
    /// Time in seconds for heat to decay to half its current value.
    /// 600s = 10 minutes, meaning a borderline-bad message (delta ~1.5)
    /// has fully decayed within an hour absent further incidents.
    double decay_half_life_seconds = 600.0;

    // Action thresholds. Must be strictly increasing.
    double censor_threshold = 1.0;
    double drop_threshold = 3.0;
    double mute_threshold = 6.0;
    double kick_threshold = 10.0;
    double ban_threshold = 15.0;
    double perma_ban_threshold = 25.0;

    // Durations for the actions that have them.
    int mute_duration_seconds = 15 * 60;
    int ban_duration_seconds = 24 * 60 * 60;

    // Per-axis weight. Multiply axis score in [0,1] by weight to get
    // heat contribution. Defaults are tuned for a roleplay-heavy
    // audience (Ace Attorney courtroom drama) where harassment-axis
    // language is expected in-character — the per-axis visibility
    // floors in ContentModerator.cpp filter sub-floor noise before
    // these weights apply, so lowering the toxicity weight here
    // doesn't make kagami soft on actual abuse (the floor handles
    // separating drama from real hostility).
    double weight_visual_noise = 0.5;
    double weight_link_risk = 5.0;
    /// Slur wordlist hits (Layer 1c). Default 6.0 means a single
    /// match crosses the 6.0 mute_threshold immediately — a first
    /// offense on a curated extremist-hate wordlist is MUTE on sight,
    /// no warnings. Lower to 3.0 for a "DROP the message, escalate
    /// on repeat" policy, or raise to 10.0 for an instant kick.
    double weight_slurs = 6.0;
    double weight_toxicity = 1.0; // roleplay-friendly; use 2.0 for general chat
    double weight_hate = 4.0;     // identity-based hate is never context-sensitive
    double weight_sexual = 1.5;
    /// Sexual content involving minors is non-negotiable: a weight high
    /// enough that any positive signal instantly crosses the perma-ban
    /// threshold. Kept as a config knob so test fixtures can override.
    double weight_sexual_minors = 100.0;
    double weight_violence = 1.0; // courtroom violence is canon; use 1.5+ for general chat
    double weight_self_harm = 1.0;
    double weight_semantic_echo = 2.0;

    /// Prune heat entries that have decayed below this value AND haven't
    /// been touched in sweep_idle_seconds. Keeps the table from growing
    /// unbounded on a long-running server.
    double prune_below = 0.05;
    int sweep_idle_seconds = 3600;
};

/// Layer 1a: visual-noise detection via unicode category analysis.
/// No dependencies, no latency, tiny false-positive surface. Catches
/// zalgo, cuneiform blasts, script-mixing homoglyphs.
struct UnicodeLayerConfig {
    bool enabled = false;

    /// Ratio of combining marks (Mn/Mc/Me) to total code points above
    /// which we consider the message zalgo-y. 0.3 = 30%.
    double combining_mark_threshold = 0.3;

    /// Ratio of exotic-script characters (Cuneiform, Tags, rarely-used
    /// CJK extensions, etc.) above which the message scores visual-noise.
    double exotic_script_threshold = 0.3;

    /// Ratio of format (Cf) or private-use (Co) characters. Format
    /// characters include RLO/LRO/ZWJ and are common attack vectors.
    double format_char_threshold = 0.1;

    /// Scalar ceiling applied to the returned score.
    double max_score = 1.0;
};

/// Layer 1b: URL extraction with optional blocklist/allowlist.
///
/// In Phase 1 this is purely offline — no domain reputation queries,
/// no Safe Browsing lookups. Just regex extraction plus a simple
/// string-match against a blocklist shipped in config. A future phase
/// can add async reputation checks.
struct UrlLayerConfig {
    bool enabled = false;

    /// Substrings to match against extracted URLs/domains. A case-
    /// insensitive substring match is used (so "bit.ly" matches any
    /// bit.ly link, and "free-robux" matches the bare domain).
    std::vector<std::string> blocklist;

    /// Substrings to whitelist. Allowlist takes precedence over blocklist.
    std::vector<std::string> allowlist;

    /// Score to assign when a blocklisted URL is found.
    double blocked_score = 1.0;

    /// Score to assign when any URL is found but it's not on the
    /// blocklist. Lets you optionally penalize links in general.
    /// 0.0 = don't penalize unknown URLs.
    double unknown_url_score = 0.0;
};

/// Layer 1c: operator-supplied slur wordlist with Scunthorpe-safe
/// word-boundary matching.
///
/// This layer exists because OpenAI's hate axis has a categorical
/// recall problem on common casual slurs — see the tuning comment
/// in ContentModerator.cpp for the scores we've measured. An
/// explicit wordlist fills the gap cheaply.
///
/// The wordlist is NOT shipped in the repo. Operators supply an
/// https URL and TextListFetcher pulls it at startup into a disk
/// cache. An empty URL leaves the layer inert regardless of `enabled`.
/// The same pattern applies to the exception list (words that would
/// otherwise trigger a false positive — reclaimed community language,
/// country/city names that normalize to a wordlist entry, etc).
struct SlurLayerConfig {
    bool enabled = false;

    /// https URL of the newline-delimited wordlist. Comments (# prefix)
    /// and blank lines are ignored. Empty URL = layer stays inert.
    std::string wordlist_url;

    /// https URL of the exception list. Empty = no exceptions; every
    /// wordlist hit fires.
    std::string exceptions_url;

    /// Filesystem directory for the cache files. TextListFetcher
    /// falls back to the cache if the URL is unreachable on startup,
    /// so Layer 1c survives a transient S3 outage.
    std::string cache_dir = "/tmp/kagami-moderation";

    /// Per-match score contributed to the slurs axis. Combined with
    /// `weight_slurs` in HeatConfig, the default 1.0 × 6.0 weight
    /// puts a single match right at the mute threshold — so a first
    /// offense crosses the ladder to MUTE immediately. Lower this
    /// (or the weight) if you want a "one warning, then action"
    /// behavior instead.
    double match_score = 1.0;
};

/// Layer 2: remote classifier (e.g. OpenAI omni-moderation).
///
/// Phase 1 declares this config shape but ContentModerator does NOT
/// yet make remote calls — Phase 2 wires it up. An empty api_key or
/// enabled=false both short-circuit the layer.
struct RemoteClassifierConfig {
    bool enabled = false;

    /// Provider name. Only "openai" is recognized today.
    std::string provider = "openai";

    /// API key. If empty, the layer is automatically disabled regardless
    /// of `enabled`, to prevent accidental deploys with a missing secret.
    std::string api_key;

    /// Endpoint URL. The default works for openai.
    std::string endpoint = "https://api.openai.com/v1/moderations";

    /// Model name sent in the request body.
    std::string model = "omni-moderation-latest";

    /// Timeout after which we give up and fall back to Layer 1 only.
    int timeout_ms = 500;

    /// If the classifier is unreachable or times out, should we let
    /// messages through (`true`, fail-open) or treat it as a failure
    /// signal (`false`, fail-closed)? Fail-open is the sane default
    /// for a chat server: better to miss a bad message than to brick
    /// the whole channel on an OpenAI incident.
    bool fail_open = true;

    /// Dedup cache for repeat messages. Eliminates duplicate OpenAI
    /// calls for identical (lightly-normalized) inputs. See
    /// RemoteDedupCache.h for the full design rationale. Opt-in to
    /// avoid quietly introducing cross-request state on existing
    /// deployments.
    bool cache_enabled = false;

    /// Max age of a cached verdict. OpenAI omni-moderation's output
    /// is stable for the same input over short intervals but can
    /// shift with policy/model updates; a TTL caps the staleness.
    int cache_ttl_seconds = 300;

    /// Max cached entries. FIFO eviction past this cap. 1000 fits in
    /// ~150 KB of RAM on a typical server.
    int cache_max_entries = 1000;
};

/// Layer 2 shortcut: embedding-similarity "safe hint" that bypasses
/// the remote classifier on messages near a known-harmless anchor.
///
/// Not a standalone layer — it short-circuits the Layer 2 call when
/// the message embeds close to any anchor. Requires the Layer 3
/// embedding backend to be loaded; without it the shortcut is inert
/// and every message goes to the remote classifier as usual.
///
/// The anchor list is operator-supplied, fetched from an https URL
/// by TextListFetcher at startup in the same background thread that
/// fetches the slur wordlist. An empty URL keeps the shortcut off.
struct SafeHintConfig {
    bool enabled = false;

    /// https URL of the newline-delimited anchor list. Each non-
    /// comment, non-blank line is one anchor phrase. Fetched once
    /// at startup, cached to disk, then run through the embedding
    /// backend to produce unit vectors.
    std::string anchors_url;

    /// Filesystem directory for the TextListFetcher cache file.
    /// Shares the same directory as the slur wordlist cache by
    /// default — both are small text lists.
    std::string cache_dir = "/tmp/kagami-moderation";

    /// Cosine similarity threshold above which the remote classifier
    /// is skipped. Default 0.7 is a moderately aggressive cutoff:
    /// tuned in staging, expected to cover most mundane chatter while
    /// letting "close but off-topic" messages still reach Layer 2.
    /// Raising it past 0.85 typically makes the shortcut useless
    /// because only near-verbatim matches qualify; dropping below
    /// 0.5 starts letting harassment through when it happens to
    /// share vocabulary with a safe anchor (not a safety issue
    /// because Layer 1 still runs, but it costs recall).
    double similarity_threshold = 0.7;
};

/// Layer 3: local embeddings for cross-message spam clustering.
///
/// Phase 1 declares this config shape but ContentModerator does NOT
/// yet load or run any model — Phase 3 wires up llama.cpp embedding
/// mode. No default model id, so the layer is completely inert until
/// an operator sets a HuggingFace model id in config.
struct EmbeddingsLayerConfig {
    bool enabled = false;

    /// HuggingFace model id, e.g. "sentence-transformers/all-MiniLM-L6-v2".
    /// Empty = layer inert. The deploy pipeline is responsible for
    /// pulling the model and placing it where the loader can find it;
    /// no model is bundled with the server binary.
    std::string hf_model_id;

    /// Filesystem directory for the downloaded GGUF model. HfModelFetcher
    /// caches the file here and short-circuits subsequent boots if the
    /// file already exists, so a persistent (bind-mounted) directory
    /// avoids re-downloading the model on every container recreate.
    /// Empty string falls back to `<binary_dir>/models` at runtime so
    /// dev/test runs without a deploy layout still work.
    std::string cache_dir = "/tmp/kagami-moderation";

    /// Ring buffer of recent embeddings across all clients, used to
    /// detect near-duplicate spam from many IPs.
    int ring_size = 500;

    /// Cosine-similarity threshold for "same message" detection.
    double similarity_threshold = 0.9;

    /// Number of distinct IPIDs that must send a near-duplicate within
    /// the window before the semantic_echo axis fires.
    int cluster_threshold = 3;

    /// Sliding window for clustering, in seconds.
    int window_seconds = 60;
};

/// Configuration for where the moderation audit log is shipped.
///
/// These sinks are completely separate from the regular Log system's
/// sinks. Each field enables/disables an independent destination, so
/// a deploy can write to sqlite only (for operators who want the
/// queryable history but nothing else), or sqlite + file + Loki, etc.
///
/// No shared credentials with the regular log sinks — the mod audit
/// stream can be split to a separate Loki label or CloudWatch stream.
/// This is deliberate: moderation events often have higher sensitivity
/// than ordinary logs and should be able to go somewhere access-
/// controlled without dragging every INFO log along with them.
struct ModerationAuditSinkConfig {
    /// Write audit events to stderr as JSON lines. Handy for dev.
    bool stdout_enabled = false;

    /// If non-empty, append audit events as JSON lines to this file.
    std::string file_path;

    /// If non-empty, POST to this Loki push endpoint.
    /// Uses its own stream label ("job=kagami_mod_audit" by default)
    /// so mod events don't mix with application logs.
    std::string loki_url;
    std::string loki_stream_label = "kagami_mod_audit";

    /// CloudWatch group/stream for audit events. All three must be set
    /// to enable. Reuses the global cloudwatch region/credentials.
    std::string cloudwatch_log_group;
    std::string cloudwatch_log_stream;

    /// Persist events to the moderation_events table. Required for
    /// the queryable history; also the cheapest and safest sink.
    bool sqlite_enabled = true;

    /// Minimum action severity to record. Events with action < this
    /// threshold are not emitted. Default = LOG, so everything non-NONE
    /// is recorded. Use "censor" to drop informational events.
    /// String form of ModerationAction.
    std::string min_action = "log";
};

/// Trust bank (negative heat) layer.
///
/// An orthogonal use of the ModerationHeat counter: users with a track
/// record of clean messages accumulate "trust credit" as NEGATIVE heat,
/// which probabilistically skips the expensive remote classifier call.
/// Any positive delta (slur, toxicity hit, etc.) resets trust to zero
/// before applying penalty — trust accelerates the skip decision, never
/// the enforcement decision.
///
/// The skip is probabilistic rather than absolute so behavioral drift
/// (a previously-trusted user going bad) is still detectable — a
/// minimum sampling rate forces ~5% of trusted messages through the
/// remote classifier for ground truth.
///
/// Disabled by default; zero behavior change from pre-trust-bank
/// kagami when off.
/// Layer 2 shortcut: local linear classifier.
///
/// Runs a thin 384x8 logistic regression on top of the existing
/// embedding vector (already computed for SafeHintLayer). High-
/// confidence positive or negative outputs short-circuit the
/// remote classifier call; uncertain middle-band outputs escalate
/// to the remote layer as usual.
///
/// See LocalClassifierLayer.h for the full design, including the
/// embedding-model compatibility check that prevents silently
/// wrong outputs when the runtime model doesn't match the model
/// the weights were trained against.
struct LocalClassifierConfig {
    bool enabled = false;

    /// If any axis score exceeds this threshold, the classifier is
    /// confident enough that the message is bad to skip the remote
    /// call. 0.9 = "very sure". Tune lower for more skips at the
    /// cost of more false positives from a model that's much less
    /// accurate than the remote classifier.
    double confidence_high_skip = 0.9;

    /// If the MAXIMUM axis score is below this threshold, the
    /// classifier is confident enough that the message is clean to
    /// skip the remote call. 0.2 = "no axis shows meaningful
    /// signal". Messages between this and confidence_high_skip
    /// escalate to the remote classifier for a tiebreaker.
    double confidence_low_clean = 0.2;
};

struct TrustBankConfig {
    bool enabled = false;

    /// Heat subtracted from the counter for each NONE-verdict (clean)
    /// message. 0.1 means 50 clean messages to reach -5.0 trust (the
    /// default api_skip_threshold).
    double clean_reward = 0.1;

    /// Maximum trust credit allowed (most-negative heat value). At
    /// -max_trust, the skip rate hits min_sample_rate. Further clean
    /// messages are no-ops.
    double max_trust = 10.0;

    /// Heat must be at most -api_skip_threshold before the trust bank
    /// is eligible to skip the remote call. Linear ramp from 100% API
    /// rate at -api_skip_threshold to min_sample_rate at -max_trust.
    double api_skip_threshold = 5.0;

    /// Floor on the sampling rate — even the most trusted users get
    /// this fraction of their messages sent to the remote classifier
    /// for ground-truth verification. 0.05 = 5% of traffic.
    double min_sample_rate = 0.05;
};

/// Top-level content moderation config. Everything defaults to off.
struct ContentModerationConfig {
    /// Master kill switch. When false, no layer runs and the moderator
    /// always returns a NONE verdict instantly.
    bool enabled = false;

    /// Whether to check IC (in-character) messages. In-character spam
    /// is less common but exists; worth enabling on public servers.
    bool check_ic = true;

    /// Whether to check OOC (out-of-character) messages.
    bool check_ooc = true;

    /// Message samples stored in the audit log are truncated to this
    /// length. Keeps the DB from ballooning and bounds any PII exposure.
    int message_sample_length = 200;

    UnicodeLayerConfig unicode;
    UrlLayerConfig urls;
    SlurLayerConfig slurs;
    RemoteClassifierConfig remote;
    SafeHintConfig safe_hint;
    EmbeddingsLayerConfig embeddings;
    HeatConfig heat;
    TrustBankConfig trust_bank;
    LocalClassifierConfig local_classifier;
    ModerationAuditSinkConfig audit;
};

} // namespace moderation
