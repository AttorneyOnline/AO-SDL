#include "moderation/ModerationAuditLog.h"

#include "utils/Log.h"

#include <json.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

namespace moderation {

void ModerationAuditLog::set_min_action(ModerationAction min) {
    std::lock_guard lock(mu_);
    min_action_ = min;
}

void ModerationAuditLog::add_sink(const std::string& name, Sink sink) {
    if (!sink)
        return;
    std::lock_guard lock(mu_);
    sinks_[name] = std::move(sink);
}

void ModerationAuditLog::remove_sink(const std::string& name) {
    std::lock_guard lock(mu_);
    sinks_.erase(name);
}

void ModerationAuditLog::clear() {
    std::lock_guard lock(mu_);
    sinks_.clear();
}

void ModerationAuditLog::record(const ModerationEvent& event) {
    // Snapshot the sink list and min-action under the lock, then
    // call sinks without holding it. Keeps fan-out latency bounded
    // by the slowest sink, but doesn't block registration.
    std::vector<Sink> snapshot;
    ModerationAction threshold;
    {
        std::lock_guard lock(mu_);
        threshold = min_action_;
        if (static_cast<int>(event.action) < static_cast<int>(threshold))
            return;
        snapshot.reserve(sinks_.size());
        for (auto& [name, sink] : sinks_)
            snapshot.push_back(sink);
    }

    for (auto& sink : snapshot) {
        try {
            sink(event);
        }
        catch (const std::exception& e) {
            Log::log_print(WARNING, "ModerationAuditLog: sink threw: %s", e.what());
        }
        catch (...) {
            Log::log_print(WARNING, "ModerationAuditLog: sink threw (unknown)");
        }
    }
}

std::vector<std::string> ModerationAuditLog::sink_names() const {
    std::lock_guard lock(mu_);
    std::vector<std::string> names;
    names.reserve(sinks_.size());
    for (auto& [name, _] : sinks_)
        names.push_back(name);
    return names;
}

void ModerationAuditLog::to_json(const ModerationEvent& event, nlohmann::json& j) {
    j = nlohmann::json{
        {"id", event.id},
        {"timestamp_ms", event.timestamp_ms},
        {"ipid", event.ipid},
        {"channel", event.channel},
        {"message_sample", event.message_sample},
        {"action", to_string(event.action)},
        {"heat_after", event.heat_after},
        {"reason", event.reason},
        {"scores",
         {
             {"visual_noise", event.scores.visual_noise},
             {"link_risk", event.scores.link_risk},
             {"toxicity", event.scores.toxicity},
             {"hate", event.scores.hate},
             {"sexual", event.scores.sexual},
             {"sexual_minors", event.scores.sexual_minors},
             {"violence", event.scores.violence},
             {"self_harm", event.scores.self_harm},
             {"semantic_echo", event.scores.semantic_echo},
         }},
    };
}

std::string ModerationAuditLog::to_json_line(const ModerationEvent& event) {
    nlohmann::json j;
    to_json(event, j);
    return j.dump();
}

// -- Built-in sinks -----------------------------------------------------------

ModerationAuditLog::Sink make_stream_sink(std::shared_ptr<std::ostream> stream) {
    if (!stream || !*stream)
        return {};
    // Serialize writes against a per-sink mutex so multiple moderation
    // events arriving concurrently don't interleave bytes on the stream.
    auto mu = std::make_shared<std::mutex>();
    return [stream = std::move(stream), mu = std::move(mu)](const ModerationEvent& event) {
        std::string line = ModerationAuditLog::to_json_line(event);
        std::lock_guard lock(*mu);
        (*stream) << line << '\n';
        stream->flush();
    };
}

ModerationAuditLog::Sink make_file_sink(const std::string& path) {
    auto ofs = std::make_shared<std::ofstream>(path, std::ios::app);
    if (!ofs->is_open()) {
        Log::log_print(WARNING, "ModerationAuditLog: could not open %s for append", path.c_str());
        return {};
    }
    return make_stream_sink(ofs);
}

ModerationAuditLog::Sink make_stderr_sink() {
    // std::cerr is a global — use a non-owning shared_ptr with a no-op
    // deleter so the stream_sink's lifecycle rules remain uniform.
    std::shared_ptr<std::ostream> stream(&std::cerr, [](std::ostream*) {});
    return make_stream_sink(std::move(stream));
}

} // namespace moderation
