#include "moderation/ContentModerator.h"

#include "asset/MountEmbedded.h"
#include "game/DatabaseManager.h"
#include "metrics/MetricsRegistry.h"
#include "moderation/ModerationTrace.h"
#include "utils/Log.h"

#include <algorithm>
#include <chrono>
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
        bad_hint_.configure(new_cfg->bad_hint);
        local_classifier_.configure(new_cfg->local_classifier);
        // Load the bundled classifier weights if the layer is on.
        // The binary is embedded via cmake/EmbedAssets.cmake at the
        // path "moderation/classifier-weights-v2.bin". If the file
        // wasn't built into this image (fresh checkout without
        // training), load_weights fails softly and the layer stays
        // inactive. The model-name header check inside load_weights
        // also disables the layer on a runtime/training mismatch.
        if (new_cfg->local_classifier.enabled) {
            const uint8_t* weights_data = nullptr;
            size_t weights_size = 0;
            for (const auto& file : embedded_assets()) {
                if (std::string_view(file.path) == "moderation/classifier-weights-v2.bin") {
                    weights_data = file.data;
                    weights_size = file.size;
                    break;
                }
            }
            if (weights_data) {
                local_classifier_.load_weights(weights_data, weights_size, new_cfg->embeddings.hf_model_id);
            }
            else {
                Log::warn("LocalClassifier: enabled in config but no bundled weights at "
                          "'moderation/classifier-weights-v2.bin' — layer will stay inert. "
                          "Run scripts/train_classifier.py and rebuild.");
            }
        }
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

void ContentModerator::set_trace_log(ModerationTraceLog* trace) {
    std::lock_guard lock(mu_);
    trace_log_ = trace;
}

void ContentModerator::set_database(DatabaseManager* db) {
    std::lock_guard lock(mu_);
    db_ = db;
}

void ContentModerator::set_action_callback(ActionCallback cb) {
    std::lock_guard lock(mu_);
    action_cb_ = std::move(cb);
}

void ContentModerator::set_embedding_backend(std::unique_ptr<EmbeddingBackend> backend) {
    embedding_backend_ = (backend && backend->is_ready()) ? backend.get() : nullptr;
    clusterer_.set_backend(std::move(backend));
}

size_t ContentModerator::set_bad_hint_anchors(const std::vector<std::string>& raw) {
    if (!embedding_backend_) {
        Log::log_print(WARNING, "ContentModerator: set_bad_hint_anchors called before embedding backend is ready; "
                                "bad-hint layer will stay inert");
        return 0;
    }
    size_t loaded = bad_hint_.load_anchors(raw, *embedding_backend_);
    Log::log_print(INFO, "ContentModerator: loaded %zu bad-hint anchors", loaded);
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

void ContentModerator::set_noheat(const std::string& ipid, bool exempt) {
    std::lock_guard lock(mu_);
    if (exempt)
        noheat_ipids_.insert(ipid);
    else
        noheat_ipids_.erase(ipid);
}

bool ContentModerator::is_noheat(const std::string& ipid) const {
    std::lock_guard lock(mu_);
    return noheat_ipids_.count(ipid) > 0;
}

std::shared_ptr<const ContentModerationConfig> ContentModerator::cfg_snapshot() const {
    std::lock_guard lock(mu_);
    return cfg_;
}

double ContentModerator::heat_delta_from(const ModerationAxisScores& s, const HeatConfig& h) {
    // Per-axis visibility floors are now configurable in HeatConfig
    // (loaded from JSON). Each axis only contributes heat when its
    // score exceeds the floor — prevents classifier noise from
    // accumulating on clean messages.
    double d = 0.0;
    if (s.visual_noise > h.floor_visual_noise)
        d += s.visual_noise * h.weight_visual_noise;
    if (s.link_risk > h.floor_link_risk)
        d += s.link_risk * h.weight_link_risk;
    if (s.slurs > h.floor_slurs)
        d += s.slurs * h.weight_slurs;
    if (s.toxicity > h.floor_toxicity)
        d += s.toxicity * h.weight_toxicity;
    if (s.hate > h.floor_hate)
        d += s.hate * h.weight_hate;
    if (s.sexual > h.floor_sexual)
        d += s.sexual * h.weight_sexual;
    if (s.sexual_minors > h.floor_sexual_minors)
        d += s.sexual_minors * h.weight_sexual_minors;
    if (s.violence > h.floor_violence)
        d += s.violence * h.weight_violence;
    if (s.self_harm > h.floor_self_harm)
        d += s.self_harm * h.weight_self_harm;
    if (s.semantic_echo > h.floor_semantic_echo)
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

ModerationVerdict ContentModerator::check(const std::string& ipid, std::string_view channel, std::string_view message,
                                          std::string_view area) {
    ModerationVerdict v;

    // Per-message telemetry trace. Always stack-allocated and
    // populated inline with the verdict, even when the trace sink
    // is disabled — the population cost is ~2-3 KB of stack for a
    // handful of scalars and two score arrays, essentially free.
    // Keeping it unconditional keeps the control flow linear and
    // lets unit tests inspect trace state directly if they want to.
    // It's only EMITTED (handed to the trace sink) at the bottom of
    // check() when trace_log_ is set and cfg->trace.enabled is true.
    ModerationTrace tr;
    tr.ipid = ipid;
    tr.channel = std::string(channel);
    tr.area = std::string(area);
    tr.message = std::string(message);
    tr.timestamp_ms = now_ms();

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
    // Bump the per-layer metric counters AND return the elapsed ns so
    // the caller can also write it into the trace sub-struct without
    // re-reading the clock. Returning the dt keeps the "time the layer,
    // record once, use twice" pattern in a single expression.
    auto bump_layer = [&](const char* layer, std::chrono::steady_clock::time_point t0) -> int64_t {
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count();
        layer_ns_counter.labels({layer}).inc(static_cast<double>(dt));
        layer_calls_counter.labels({layer}).inc();
        return dt;
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
        tr.unicode.ran = true;
        tr.unicode.visual_noise = unicode_result.score;
        tr.unicode.ns = bump_layer("unicode", t0);
    }

    // --- Layer 1b: URL blocklist ------------------------------------
    {
        auto t0 = std::chrono::steady_clock::now();
        auto url_result = urls_.extract(message);
        v.scores.link_risk = url_result.score;
        if (url_result.score > 0.0) {
            v.triggered_axes.push_back("link(" + url_result.reason + ")");
        }
        tr.urls.ran = true;
        tr.urls.link_risk = url_result.score;
        tr.urls.urls = url_result.urls; // full extracted list, for human review
        tr.urls.ns = bump_layer("urls", t0);
    }

    // --- Layer 1c: slur wordlist ------------------------------------
    // Word-boundary match against an operator-supplied wordlist after
    // a Scunthorpe-safe normalizer pass. Runs before Layer 2 because:
    //
    //   1. It's strictly local — no network, no rate-limit concerns.
    //   2. The remote classifier misses this class of content. OpenAI
    //      scores common pro-Nazi phrases at hate=0.00014 and various
    //      casual racial epithets in the 0.05-0.3 range, which can't
    //      be caught by any sensible floor tweak. This layer fills
    //      that gap.
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
        tr.slurs.ran = true;
        tr.slurs.match_score = slur_result.score;
        tr.slurs.matches = slur_result.matched;
        tr.slurs.ns = bump_layer("slurs", t0);
    }

    // --- Layer 2: local MLP classifier + bad-hint --------------------
    //
    // The local classifier is the PRIMARY moderation classifier. No
    // remote API calls — the MLP runs on the bge-small embedding
    // (~10µs), produces per-axis scores, and those scores feed
    // directly into the heat ladder via per-axis floors + weights.
    //
    // BadHintLayer provides supplemental detection by matching against
    // operator-curated anchor phrases, plugging recall holes in the
    // classifier without retraining.
    //
    // Keysmash suppression: messages with near-zero vowel ratio (e.g.
    // "KFVIOULPJBFVSDJHLGBUKDLSFGFD") produce garbage embeddings that
    // the MLP makes noisy predictions on. Suppress classifier scores
    // to zero for these — they're not meaningful content, and Layer 1
    // UnicodeClassifier handles the visual-noise aspect.
    {
        // --- Keysmash detection ------------------------------------
        bool keysmash_suppressed = false;
        {
            int alpha = 0, vowels = 0;
            for (char c : message) {
                char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (std::isalpha(static_cast<unsigned char>(lower))) {
                    alpha++;
                    if (lower == 'a' || lower == 'e' || lower == 'i' || lower == 'o' || lower == 'u')
                        vowels++;
                }
            }
            if (alpha >= 8 && static_cast<double>(vowels) / alpha < 0.25)
                keysmash_suppressed = true;
        }
        tr.keysmash_suppressed = keysmash_suppressed;

        // --- Compute shared embedding ONCE -------------------------
        EmbeddingResult shared_embedding;
        const bool need_embedding =
            embedding_backend_ && (local_classifier_.is_active() || bad_hint_.is_active());
        if (need_embedding) {
            auto t_emb = std::chrono::steady_clock::now();
            shared_embedding = embedding_backend_->embed(message);
            tr.layer2_embedding.ran = shared_embedding.ok;
            tr.layer2_embedding.dim = static_cast<int>(shared_embedding.vector.size());
            tr.layer2_embedding.ns = bump_layer("layer2_embedding", t_emb);
        }

        // --- Local MLP classifier ----------------------------------
        if (local_classifier_.is_active() && shared_embedding.ok) {
            auto t_lc = std::chrono::steady_clock::now();
            auto lc_result = local_classifier_.classify(shared_embedding);
            tr.local_classifier.ran = lc_result.ran;
            tr.local_classifier.scores = lc_result.scores;
            tr.local_classifier.max_confidence = lc_result.max_confidence;
            tr.local_classifier.max_category_index = lc_result.max_category_index;
            tr.local_classifier.ns = bump_layer("local_classifier", t_lc);

            if (lc_result.ran && !keysmash_suppressed) {
                // Merge ALL classifier scores into the verdict. No
                // confidence gating — per-axis floors in heat_delta_from()
                // handle the "is this score meaningful?" decision.
                v.scores.toxicity = std::max(v.scores.toxicity, lc_result.scores.toxicity);
                v.scores.hate = std::max(v.scores.hate, lc_result.scores.hate);
                v.scores.sexual = std::max(v.scores.sexual, lc_result.scores.sexual);
                v.scores.sexual_minors = std::max(v.scores.sexual_minors, lc_result.scores.sexual_minors);
                v.scores.violence = std::max(v.scores.violence, lc_result.scores.violence);
                v.scores.self_harm = std::max(v.scores.self_harm, lc_result.scores.self_harm);

                // Tag axes that crossed their visibility floor.
                if (lc_result.scores.toxicity > cfg->heat.floor_toxicity)
                    v.triggered_axes.emplace_back("toxicity");
                if (lc_result.scores.hate > cfg->heat.floor_hate)
                    v.triggered_axes.emplace_back("hate");
                if (lc_result.scores.sexual > cfg->heat.floor_sexual)
                    v.triggered_axes.emplace_back("sexual");
                if (lc_result.scores.sexual_minors > cfg->heat.floor_sexual_minors)
                    v.triggered_axes.emplace_back("sexual_minors");
                if (lc_result.scores.violence > cfg->heat.floor_violence)
                    v.triggered_axes.emplace_back("violence");
                if (lc_result.scores.self_harm > cfg->heat.floor_self_harm)
                    v.triggered_axes.emplace_back("self_harm");
            }
        }

        // --- Bad-hint anchor detection -----------------------------
        if (bad_hint_.is_active() && shared_embedding.ok) {
            auto t_bh = std::chrono::steady_clock::now();
            auto bh_result = bad_hint_.query_with_embedding(shared_embedding);
            tr.bad_hint.ran = true;
            tr.bad_hint.max_similarity = bh_result.max_similarity;
            tr.bad_hint.best_anchor_index = bh_result.best_anchor_index;
            tr.bad_hint.is_bad = bh_result.is_bad;
            tr.bad_hint.ns = bump_layer("bad_hint", t_bh);

            if (bh_result.is_bad) {
                const std::string& axis = cfg->bad_hint.inject_axis;
                const double score = cfg->bad_hint.inject_score;
                if (axis == "toxicity")
                    v.scores.toxicity = std::max(v.scores.toxicity, score);
                else if (axis == "hate")
                    v.scores.hate = std::max(v.scores.hate, score);
                else if (axis == "sexual")
                    v.scores.sexual = std::max(v.scores.sexual, score);
                else if (axis == "sexual_minors")
                    v.scores.sexual_minors = std::max(v.scores.sexual_minors, score);
                else if (axis == "violence")
                    v.scores.violence = std::max(v.scores.violence, score);
                else if (axis == "self_harm")
                    v.scores.self_harm = std::max(v.scores.self_harm, score);
                v.triggered_axes.emplace_back(axis + "_bh");
            }
        }
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
        tr.semantic_cluster.ran = true;
        tr.semantic_cluster.semantic_echo = sr.score;
        // distinct_ipids is the number of unique users whose
        // recent messages matched — the right signal for
        // gap-hunting (1 = singleton, 3+ = genuine cross-user
        // echo event worth flagging).
        tr.semantic_cluster.cluster_size = sr.distinct_ipids;
        tr.semantic_cluster.ns = bump_layer("embeddings", t0);
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

    // Emit the trace to the telemetry sink if enabled. Called from
    // both the early-NONE return below and the main return at the
    // bottom of check(). Populates the decision fields from the
    // current verdict state, so it must run AFTER the verdict is
    // finalized (action, heat_after, reason all set). Cheap when
    // trace_log_ is null or disabled — a single branch + early out.
    auto emit_trace_if_enabled = [&]() {
        if (!trace_log_ || !cfg->trace.enabled)
            return;
        tr.final_scores = v.scores;
        tr.heat_delta = v.heat_delta;
        tr.heat_after = v.heat_after;
        tr.final_action = v.action;
        tr.reason = v.reason;
        tr.triggered_axes = v.triggered_axes;
        trace_log_->record(tr);
    };

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
        emit_trace_if_enabled();
        // Wall-clock timing for the whole check() including the early
        // return path (so dashboards see the full cost, not just the
        // action-bearing subset).
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - check_start)
                      .count();
        check_ns_counter.get().inc(static_cast<uint64_t>(dt));
        check_calls_counter.get().inc();
        return v;
    }

    // --- Noheat exemption check --------------------------------------
    // If the IPID has noheat mode on, we still compute what WOULD
    // happen (for traces/audit) but don't apply heat or enforce.
    const bool noheat = is_noheat(ipid);
    if (noheat) {
        // Simulate: compute hypothetical heat and action for logging
        const double hypothetical_heat = heat_.peek(ipid) + v.heat_delta;
        v.heat_after = heat_.peek(ipid); // actual heat unchanged
        v.action = heat_.decide(hypothetical_heat); // what would have happened
        v.reason = format_reason(v);
        if (!v.reason.empty())
            v.reason += " [noheat_suppressed]";
        tr.noheat_suppressed = true;
    }
    else {
        // Real content — apply the delta and determine action.
        v.heat_after = heat_.apply(ipid, v.heat_delta);
        v.action = heat_.decide(v.heat_after);
        v.reason = format_reason(v);
    }

    // Per-action event counter (low cardinality: <10 actions × 2 channels).
    events_counter.labels({to_string(v.action), std::string(channel)}).inc();

    // Per-axis fires. Uses the same per-axis floors as
    // heat_delta_from() so "fired" and "added heat" are always
    // the same event in the dashboards.
    if (v.scores.visual_noise > cfg->heat.floor_visual_noise)
        axis_fires.labels({"visual_noise"}).inc();
    if (v.scores.link_risk > cfg->heat.floor_link_risk)
        axis_fires.labels({"link_risk"}).inc();
    if (v.scores.slurs > cfg->heat.floor_slurs)
        axis_fires.labels({"slurs"}).inc();
    if (v.scores.toxicity > cfg->heat.floor_toxicity)
        axis_fires.labels({"toxicity"}).inc();
    if (v.scores.hate > cfg->heat.floor_hate)
        axis_fires.labels({"hate"}).inc();
    if (v.scores.sexual > cfg->heat.floor_sexual)
        axis_fires.labels({"sexual"}).inc();
    if (v.scores.sexual_minors > cfg->heat.floor_sexual_minors)
        axis_fires.labels({"sexual_minors"}).inc();
    if (v.scores.violence > cfg->heat.floor_violence)
        axis_fires.labels({"violence"}).inc();
    if (v.scores.self_harm > cfg->heat.floor_self_harm)
        axis_fires.labels({"self_harm"}).inc();
    if (v.scores.semantic_echo > cfg->heat.floor_semantic_echo)
        axis_fires.labels({"semantic_echo"}).inc();

    // --- Side effects -----------------------------------------------
    // Skip all enforcement when noheat is active. Traces and metrics
    // above still recorded what WOULD have happened.
    if (noheat) {
        emit_trace_if_enabled();
        auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now() - check_start)
                      .count();
        check_ns_counter.get().inc(static_cast<uint64_t>(dt));
        check_calls_counter.get().inc();
        // Return the verdict with the hypothetical action so the
        // caller sees what would have happened, but the action has
        // no enforcement backing. Packet handler should still
        // broadcast the message normally.
        v.action = ModerationAction::NONE;
        return v;
    }

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

    // Emit the per-message trace after the verdict is final and
    // the audit log has been fanned out — the trace is a separate
    // stream with a different consumer set (Loki + JSONL file) so
    // it doesn't share any state with the audit path above.
    emit_trace_if_enabled();

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
