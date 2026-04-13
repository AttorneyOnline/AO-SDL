/**
 * @file ModerationTrace.h
 * @brief Per-message telemetry describing each moderation layer's
 *        contribution to a single check() call.
 *
 * Why this exists
 * ---------------
 * The counter metrics (`kagami_moderation_layer_calls_total`,
 * `kagami_moderation_layer2_skipped_total`) tell you how often each
 * layer ran and how often each skip reason fired in aggregate.
 * They're the right tool for ops dashboards — rates, SLIs, cost
 * tracking.
 *
 * ModerationTrace is the opposite shape: one structured record per
 * individual message, showing exactly what every layer observed and
 * how the final decision was reached. The intended consumers are:
 *
 *   1. Loki + Grafana. A trace stream (label `kagami_mod_trace`) is
 *      queryable via LogQL: "find all messages where the local
 *      classifier said max_conf > 0.8 but the remote classifier
 *      disagreed", "sort paraphrased-hate candidates by bad_hint
 *      similarity score", "what's the p50 classifier confidence on
 *      clean traffic this week".
 *   2. Local JSONL file (`/logs/kagami_mod_trace.jsonl`). Offline
 *      grep-and-inspect. Survives Loki pipeline outages.
 *   3. Human review. An operator looking at a mute decision can
 *      pull up the trace and see which layer fired, what score it
 *      produced, whether the classifier agreed with the remote,
 *      etc. — the full reasoning, not just the final action.
 *
 * Not an audit log replacement. ModerationAuditLog / ModerationEvent
 * still exists and is the source of truth for enforcement records
 * (what mutes/bans were applied, when, for what reason). The trace
 * is additive — populated alongside the verdict and emitted to
 * dedicated trace sinks separate from audit sinks.
 *
 * Thread safety: the trace is populated synchronously from within
 * ContentModerator::check() on a single thread (the one running the
 * check). Once complete it is moved into the emit path. No internal
 * synchronization needed.
 */
#pragma once

#include "moderation/ModerationTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace moderation {

// ---------------------------------------------------------------------------
// Per-layer contribution structs
// ---------------------------------------------------------------------------
//
// Every layer in the moderation stack gets its own sub-struct. All
// fields except `ran` and `ns` are optional so the JSON serializer can
// omit unset fields — a layer that didn't run emits `{"ran":false}`
// rather than a wall of zeros.
//
// `ran` == true means the layer executed AND produced a result that
// contributed to the decision. `ran` == false means the layer was
// disabled, inert, or skipped due to a prior short-circuit.
//
// `ns` is the wall-clock nanoseconds the layer spent when it ran, or
// 0 when it didn't. Included alongside the contribution so Grafana
// can join "this layer said X" with "this layer took Y ns" for
// per-layer performance sorting in the same query.

struct UnicodeLayerTrace {
    bool ran = false;
    int64_t ns = 0;
    double visual_noise = 0.0;
};

struct UrlsLayerTrace {
    bool ran = false;
    int64_t ns = 0;
    double link_risk = 0.0;
    /// URLs extracted from the message — bounded list, since
    /// most messages contain 0-3 URLs at most. Helps ops curate
    /// the allowlist/blocklist over time.
    std::vector<std::string> urls;
};

struct SlursLayerTrace {
    bool ran = false;
    int64_t ns = 0;
    double match_score = 0.0;
    /// Matched slur tokens (post-normalization), bounded to the
    /// number of hits found in the message.
    std::vector<std::string> matches;
};

struct Layer2EmbeddingTrace {
    bool ran = false;
    int64_t ns = 0;
    /// Dimensionality of the produced vector (384 for bge-small,
    /// 0 when not produced). Sanity checker for cross-model drift.
    int dim = 0;
};

struct LocalClassifierTrace {
    bool ran = false;
    int64_t ns = 0;
    /// Per-axis sigmoid outputs. When the classifier didn't run,
    /// these are all zero; Grafana queries should gate on `ran`.
    ModerationAxisScores scores;
    /// Maximum per-axis confidence across trained axes.
    double max_confidence = 0.0;
    /// Index into the axis list of the argmax axis. -1 when
    /// nothing was computed.
    int max_category_index = -1;
};

struct BadHintTrace {
    bool ran = false;
    int64_t ns = 0;
    double max_similarity = 0.0;
    int best_anchor_index = -1;
    bool is_bad = false;
};

struct SemanticClusterTrace {
    bool ran = false;
    int64_t ns = 0;
    double semantic_echo = 0.0;
    int cluster_size = 0;
};

// ---------------------------------------------------------------------------
// Top-level trace envelope
// ---------------------------------------------------------------------------

struct ModerationTrace {
    /// Wall-clock milliseconds since epoch, same clock as ModerationEvent.
    int64_t timestamp_ms = 0;
    std::string ipid;
    std::string channel; ///< "ic" or "ooc"
    std::string area;    ///< Area/room name for context in traces

    /// Full message text. Bounded by the kagami IC/OOC length limit
    /// (256 chars default), so no truncation needed here.
    std::string message;

    // Per-layer contributions in Layer 1 → Layer 2 → Layer 3 order.
    UnicodeLayerTrace unicode;
    UrlsLayerTrace urls;
    SlursLayerTrace slurs;
    Layer2EmbeddingTrace layer2_embedding;
    LocalClassifierTrace local_classifier;
    BadHintTrace bad_hint;
    SemanticClusterTrace semantic_cluster;

    /// True if the message was suppressed as OOD keyboard mash (low
    /// vowel ratio). The classifier still ran (scores are in
    /// local_classifier) but the scores were NOT merged into the
    /// verdict because they're noise on a garbage embedding.
    bool keysmash_suppressed = false;

    /// True if the IPID was in noheat exemption mode. The full
    /// pipeline ran and the action was determined, but heat was NOT
    /// applied and enforcement was NOT executed. Traces show what
    /// WOULD have happened.
    bool noheat_suppressed = false;

    // -------------------------------------------------------------------
    // Decision outputs (legacy skip_reason kept for trace JSON compat)
    // -------------------------------------------------------------------
    std::string skip_reason;

    /// Axis names that crossed their per-axis visibility floor.
    /// Same set used for ModerationVerdict::triggered_axes, copied
    /// here for self-containment.
    std::vector<std::string> triggered_axes;

    /// Final merged scores after all layers, matching ModerationVerdict.
    ModerationAxisScores final_scores;
    double heat_before = 0.0;
    double heat_delta = 0.0;
    double heat_after = 0.0;
    ModerationAction final_action = ModerationAction::NONE;
    /// Human-readable reason copied from ModerationVerdict.
    std::string reason;
};

/// Serialize a ModerationTrace to a JSON line (no trailing newline).
/// Format is stable and designed to be LogQL-friendly: flat-ish
/// per-layer objects so Loki's `| json layer_field="layers.foo.bar"`
/// extraction works without nested key paths blowing up the index.
std::string trace_to_json_line(const ModerationTrace& trace);

} // namespace moderation
