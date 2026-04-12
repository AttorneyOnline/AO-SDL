#include "moderation/ModerationTrace.h"

#include <json.hpp>

#include <string>

namespace moderation {

namespace {

// Render a ModerationAxisScores as a compact JSON object. Axes are
// always emitted (even when zero) so LogQL queries on e.g.
// `scores.hate` always find the field — missing-vs-zero ambiguity
// would make per-axis range queries unreliable.
nlohmann::json scores_to_json(const ModerationAxisScores& s) {
    return nlohmann::json{
        {"visual_noise", s.visual_noise},   {"link_risk", s.link_risk}, {"slurs", s.slurs},
        {"toxicity", s.toxicity},           {"hate", s.hate},           {"sexual", s.sexual},
        {"sexual_minors", s.sexual_minors}, {"violence", s.violence},   {"self_harm", s.self_harm},
        {"semantic_echo", s.semantic_echo},
    };
}

const char* action_to_string(ModerationAction a) {
    switch (a) {
    case ModerationAction::NONE:
        return "NONE";
    case ModerationAction::LOG:
        return "LOG";
    case ModerationAction::CENSOR:
        return "CENSOR";
    case ModerationAction::DROP:
        return "DROP";
    case ModerationAction::MUTE:
        return "MUTE";
    case ModerationAction::KICK:
        return "KICK";
    case ModerationAction::BAN:
        return "BAN";
    case ModerationAction::PERMA_BAN:
        return "PERMA_BAN";
    }
    return "UNKNOWN";
}

} // namespace

std::string trace_to_json_line(const ModerationTrace& trace) {
    // Build the top-level object. Keeping this hand-written rather
    // than reflection-based so the exact field order and presence
    // rules are obvious at review time — LogQL compatibility matters
    // more than C++ terseness here.
    nlohmann::json j;
    j["ts"] = trace.timestamp_ms;
    j["ipid"] = trace.ipid;
    j["channel"] = trace.channel;
    j["area"] = trace.area;
    j["message"] = trace.message;

    // Each layer is an object whose first field is always `ran`. The
    // serializer emits zero values when !ran so LogQL queries can
    // always probe the same field paths; the `ran` flag tells the
    // consumer whether those values are meaningful. Alternative —
    // omitting fields on !ran — would force every LogQL query to
    // also check `ran`, which is error-prone.

    j["layers"]["unicode"] = {
        {"ran", trace.unicode.ran},
        {"ns", trace.unicode.ns},
        {"visual_noise", trace.unicode.visual_noise},
    };

    {
        nlohmann::json u = {
            {"ran", trace.urls.ran},
            {"ns", trace.urls.ns},
            {"link_risk", trace.urls.link_risk},
        };
        u["urls"] = trace.urls.urls;
        j["layers"]["urls"] = std::move(u);
    }

    {
        nlohmann::json s = {
            {"ran", trace.slurs.ran},
            {"ns", trace.slurs.ns},
            {"match_score", trace.slurs.match_score},
        };
        s["matches"] = trace.slurs.matches;
        j["layers"]["slurs"] = std::move(s);
    }

    j["layers"]["layer2_embedding"] = {
        {"ran", trace.layer2_embedding.ran},
        {"ns", trace.layer2_embedding.ns},
        {"dim", trace.layer2_embedding.dim},
    };

    j["layers"]["local_classifier"] = {
        {"ran", trace.local_classifier.ran},
        {"ns", trace.local_classifier.ns},
        {"max_confidence", trace.local_classifier.max_confidence},
        {"max_category_index", trace.local_classifier.max_category_index},
        {"scores", scores_to_json(trace.local_classifier.scores)},
    };

    j["layers"]["bad_hint"] = {
        {"ran", trace.bad_hint.ran},
        {"ns", trace.bad_hint.ns},
        {"max_similarity", trace.bad_hint.max_similarity},
        {"best_anchor_index", trace.bad_hint.best_anchor_index},
        {"is_bad", trace.bad_hint.is_bad},
    };

    // safe_hint, trust_bank, and remote layers removed in MLP v2
    // (no remote API calls). Omitting from trace JSON to keep payload
    // small. Loki queries referencing these old fields will return null.

    j["layers"]["semantic_cluster"] = {
        {"ran", trace.semantic_cluster.ran},
        {"ns", trace.semantic_cluster.ns},
        {"semantic_echo", trace.semantic_cluster.semantic_echo},
        {"cluster_size", trace.semantic_cluster.cluster_size},
    };

    // Decision payload — same shape as ModerationEvent plus the
    // skip_reason and heat_before which are unique to the trace.
    j["decision"] = {
        {"keysmash_suppressed", trace.keysmash_suppressed},
        {"noheat_suppressed", trace.noheat_suppressed},
        {"triggered_axes", trace.triggered_axes},
        {"final_scores", scores_to_json(trace.final_scores)},
        {"heat_before", trace.heat_before},
        {"heat_delta", trace.heat_delta},
        {"heat_after", trace.heat_after},
        {"final_action", action_to_string(trace.final_action)},
        {"reason", trace.reason},
    };

    // Compact dump with UTF-8 error replacement so a message
    // containing malformed bytes doesn't throw. Same policy as
    // RemoteClassifier's request body encoding.
    return j.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

} // namespace moderation
