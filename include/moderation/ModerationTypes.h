/**
 * @file ModerationTypes.h
 * @brief Shared types for the content moderation subsystem.
 *
 * The moderation subsystem is layered: cheap rule-based checks
 * (unicode shape, URL blocklist) run synchronously, while a remote
 * classifier and optional local embeddings can be enabled to feed
 * additional signals into the same heat accumulator.
 *
 * This header defines:
 *   - ModerationAction       — ordered ladder of responses
 *   - ModerationAxisScores   — per-axis floating-point signals
 *   - ModerationVerdict      — the decision returned by ContentModerator
 *   - ModerationEvent        — the structured record emitted to the audit log
 *
 * All moderation calls are intentionally stateless from the caller's
 * perspective: you pass a message, you get a verdict, and the moderator
 * mutates its internal heat table as a side effect.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace moderation {

/// Ordered ladder of responses to abusive content. Higher ordinal =
/// more severe. ContentModerator::check() returns exactly one of these;
/// the caller is responsible for actually applying it (sending a sanitized
/// message, dropping silently, kicking, etc.).
///
/// `NONE` and `LOG` are both "let the message through" — `LOG` just means
/// an event was recorded for audit. `CENSOR` means the caller should
/// broadcast the sanitized_message field rather than the original.
enum class ModerationAction : int {
    NONE = 0,    ///< No moderation action; broadcast unchanged.
    LOG = 1,     ///< Broadcast unchanged but record an audit event.
    CENSOR = 2,  ///< Broadcast ModerationVerdict::sanitized_message.
    DROP = 3,    ///< Silently drop; sender sees their message, nobody else does.
    MUTE = 4,    ///< Apply a temporary mute for the configured duration.
    KICK = 5,    ///< Disconnect the client with a reason.
    BAN = 6,     ///< Apply a timed ban via BanManager.
    PERMA_BAN = 7, ///< Apply a permanent ban (duration=-2 in akashi schema).
};

/// String form of a ModerationAction, suitable for logs and DB rows.
inline const char* to_string(ModerationAction a) {
    switch (a) {
    case ModerationAction::NONE: return "none";
    case ModerationAction::LOG: return "log";
    case ModerationAction::CENSOR: return "censor";
    case ModerationAction::DROP: return "drop";
    case ModerationAction::MUTE: return "mute";
    case ModerationAction::KICK: return "kick";
    case ModerationAction::BAN: return "ban";
    case ModerationAction::PERMA_BAN: return "perma_ban";
    }
    return "unknown";
}

/// Per-axis floating-point signals. Each axis is in [0.0, 1.0] where
/// 0 means "definitely fine" and 1 means "definitely bad on this axis".
/// Axes are independent: a message can score high on multiple axes.
///
/// Layer 1 (rules) populates visual_noise and link_risk.
/// Layer 2 (remote classifier) populates toxicity through self_harm.
/// Layer 3 (embeddings) populates semantic_echo.
struct ModerationAxisScores {
    // Layer 1: rule-based
    double visual_noise = 0.0;   ///< Zalgo, exotic unicode, script-mixing.
    double link_risk = 0.0;      ///< URLs against a blocklist or suspicious TLDs.

    // Layer 2: remote classifier
    double toxicity = 0.0;       ///< General harassment / abuse.
    double hate = 0.0;           ///< Identity-based hate (covers slurs).
    double sexual = 0.0;         ///< Sexual content (adult).
    double sexual_minors = 0.0;  ///< Sexual content involving minors. Always catastrophic.
    double violence = 0.0;       ///< Violent imagery or threats.
    double self_harm = 0.0;      ///< Self-harm encouragement.

    // Layer 3: cross-message clustering
    double semantic_echo = 0.0;  ///< Near-duplicate of recent messages from other IPs.
};

/// The decision ContentModerator returns for a single message.
struct ModerationVerdict {
    ModerationAction action = ModerationAction::NONE;
    ModerationAxisScores scores;

    double heat_delta = 0.0;   ///< Heat contribution from this message alone.
    double heat_after = 0.0;   ///< Accumulator value after this message was applied.

    /// If action == CENSOR, this holds the version to broadcast.
    /// Otherwise empty.
    std::string sanitized_message;

    /// Human-readable summary of which axes fired and why.
    /// Intended for the audit log, not user-facing.
    std::string reason;

    /// Axes that contributed nonzero heat, in string form.
    /// Used for metric labels and audit-log searchability.
    std::vector<std::string> triggered_axes;
};

/// A structured audit-log record, emitted for every moderation decision
/// (including `NONE` and `LOG`) when auditing is enabled. The same record
/// is persisted to sqlite for the queryable history, and serialized as
/// JSON for any external sinks (Loki, CloudWatch, file, stdout).
struct ModerationEvent {
    int64_t id = 0;                     ///< Row id after DB insert; 0 before.
    int64_t timestamp_ms = 0;           ///< Wall-clock milliseconds since epoch.
    std::string ipid;                   ///< Privacy-preserving client id.
    std::string channel;                ///< "ic" or "ooc".
    std::string message_sample;         ///< Message, possibly truncated.
    ModerationAxisScores scores;
    ModerationAction action = ModerationAction::NONE;
    double heat_after = 0.0;
    std::string reason;
};

} // namespace moderation
