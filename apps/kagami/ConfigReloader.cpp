#include "ConfigReloader.h"
#include "ContentConfig.h"
#include "ServerContext.h"
#include "ServerSettings.h"

#include "game/GameRoom.h"
#include "moderation/ContentModerator.h"
#include "net/RateLimiter.h"
#include "net/RestRouter.h"
#include "net/WebSocketServer.h"
#include "net/nx/NXServer.h"
#include "utils/Log.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

// ---------------------------------------------------------------------------
// Restart-required key prefixes. Changes to these are reported but not applied
// because the relevant resources (sockets, log sinks, background fetch threads)
// are bound at startup.
// ---------------------------------------------------------------------------
static const std::vector<std::string> kRestartKeys = {
    "bind_address",
    "http_port",
    "ws_port",
    "wss_port",
    "domain",
    "log_level",
    "log_file",
    "log_file_level",
    "loki_url",
    "cloudwatch",
    "reverse_proxy",
    "advertiser",
    "deploy",
    "content_moderation/embeddings/hf_model_id",
    "content_moderation/slurs/wordlist_url",
    "content_moderation/slurs/exceptions_url",
    "content_moderation/bad_hint/anchors_url",
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Snapshot a config value as a JSON string for comparison / display.
static std::string snapshot_value(const ServerSettings& cfg, const std::string& key) {
    auto raw = cfg.value<nlohmann::json>(key);
    if (raw.is_null())
        return {};
    return raw.dump();
}

/// Apply content config changes inside the dispatch lock.
static void apply_content_changes(GameRoom& room, const ContentConfig& new_content,
                                  const std::vector<std::string>& old_characters,
                                  const std::vector<std::string>& old_areas, ReloadResult& result) {
    // --- Relocate players from removed areas ---
    std::unordered_set<std::string> new_area_set(new_content.area_names.begin(), new_content.area_names.end());
    std::string safe_area = new_content.area_names.empty() ? "" : new_content.area_names[0];

    for (const auto& old_area : old_areas) {
        if (new_area_set.count(old_area) == 0) {
            auto sessions = room.sessions_in_area(old_area);
            for (auto* session : sessions) {
                session->area = safe_area;
                result.players_relocated++;
            }
        }
    }

    // --- Unselect players using removed characters ---
    std::unordered_set<std::string> new_char_set(new_content.characters.begin(), new_content.characters.end());

    // Build a map from old character name -> session list (for re-mapping later).
    // Also unselect anyone whose character was removed.
    room.for_each_session([&](ServerSession& session) {
        if (session.character_id < 0 || session.character_id >= static_cast<int>(old_characters.size()))
            return;
        const auto& char_name = old_characters[session.character_id];
        if (new_char_set.count(char_name) == 0) {
            session.character_id = -1;
            result.characters_unselected++;
        }
    });

    // --- Preserve area runtime state for surviving areas ---
    // Snapshot by name (not id, since id is a derived hash).
    std::unordered_map<std::string, AreaState> preserved;
    for (auto& [id, state] : room.area_states_mut())
        preserved.emplace(state.name, std::move(state));

    // --- Replace content lists ---
    room.characters = new_content.characters;
    room.music = new_content.music;
    room.areas = new_content.area_names;

    // --- Rebuild indexes ---
    room.reset_taken();
    room.build_char_id_index();
    room.build_area_index();

    // --- Restore preserved area state ---
    for (auto& [id, state] : room.area_states_mut()) {
        auto it = preserved.find(state.name);
        if (it != preserved.end()) {
            auto& old = it->second;
            state.status = std::move(old.status);
            state.lock_mode = old.lock_mode;
            state.background = std::move(old.background);
            state.music = std::move(old.music);
            state.hp = old.hp;
            state.timers = std::move(old.timers);
            state.evidence = std::move(old.evidence);
            state.testimony = std::move(old.testimony);
            state.cm_owners = std::move(old.cm_owners);
            state.cm = std::move(old.cm);
            state.invited = std::move(old.invited);
            state.bg_locked = old.bg_locked;
        }
    }

    // --- Apply per-area settings from areas.ini ---
    for (const auto& ac : new_content.area_configs) {
        auto* area = room.find_area_by_name(ac.name);
        if (!area)
            continue;
        // Only apply background from ini if area state wasn't preserved
        // (preserved areas keep their runtime background).
        if (preserved.count(ac.name) == 0)
            area->background.name = ac.background;
        area->bg_locked = ac.bg_locked;
    }

    // --- Re-map surviving character selections ---
    // Build name -> new index map for O(1) lookup.
    std::unordered_map<std::string, int> new_char_index;
    for (int i = 0; i < static_cast<int>(new_content.characters.size()); ++i)
        new_char_index[new_content.characters[i]] = i;

    room.for_each_session([&](ServerSession& session) {
        if (session.character_id < 0)
            return;
        if (session.character_id >= static_cast<int>(old_characters.size())) {
            session.character_id = -1;
            return;
        }
        const auto& char_name = old_characters[session.character_id];
        auto it = new_char_index.find(char_name);
        if (it != new_char_index.end()) {
            session.character_id = it->second;
            if (it->second < static_cast<int>(room.char_taken.size()))
                room.char_taken[it->second] = 1;
        }
        else {
            session.character_id = -1;
        }
    });

    // --- Notify backends ---
    room.broadcast_chars_taken();
}

// ---------------------------------------------------------------------------
// ReloadResult::format
// ---------------------------------------------------------------------------

std::string ReloadResult::format() const {
    std::ostringstream out;
    if (!ok) {
        out << "[reload] Error: " << error << "\n";
        return out.str();
    }

    for (const auto& line : reloaded)
        out << "[reload] " << line << "\n";

    if (players_relocated > 0)
        out << "[reload] " << players_relocated << " player(s) relocated from removed areas\n";
    if (characters_unselected > 0)
        out << "[reload] " << characters_unselected << " player(s) unselected from removed characters\n";

    if (!restart_warnings.empty()) {
        out << "[reload] Restart required for:\n";
        for (const auto& w : restart_warnings)
            out << "  " << w << "\n";
    }

    if (content_changed)
        out << "[reload] Note: existing AO2 clients see new content on reconnect\n";

    return out.str();
}

// ---------------------------------------------------------------------------
// perform_reload (explicit instantiation at bottom)
// ---------------------------------------------------------------------------

ReloadResult perform_reload(ServerContext& ctx, LockWrapper lock_wrapper) {
    ReloadResult result;
    auto& cfg = ctx.cfg;

    // -----------------------------------------------------------------------
    // Phase 1: Read files from disk (no locks held)
    // -----------------------------------------------------------------------
    std::vector<uint8_t> json_bytes;
    {
        std::ifstream file(ctx.cfg_path, std::ios::binary);
        if (!file.is_open()) {
            result.ok = false;
            result.error = "Could not open " + ctx.cfg_path;
            return result;
        }
        json_bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    // Validate JSON before touching any state.
    {
        auto test = nlohmann::json::parse(json_bytes, nullptr, false);
        if (test.is_discarded() || !test.is_object()) {
            result.ok = false;
            result.error = "Failed to parse " + ctx.cfg_path + " as JSON";
            return result;
        }
    }

    ContentConfig new_content;
    bool content_loaded = false;
    if (!ctx.content_dir.empty())
        content_loaded = new_content.load(ctx.content_dir);

    // -----------------------------------------------------------------------
    // Phase 2: Diff restart-required keys (before applying new config)
    // -----------------------------------------------------------------------
    std::unordered_map<std::string, std::string> old_snapshots;
    for (const auto& key : kRestartKeys)
        old_snapshots[key] = snapshot_value(cfg, key);

    // Apply the new JSON into ServerSettings (thread-safe via shared_mutex).
    if (!cfg.deserialize(json_bytes)) {
        result.ok = false;
        result.error = "ServerSettings::deserialize() failed";
        return result;
    }
    result.reloaded.push_back("Re-read " + ctx.cfg_path);

    // Check restart-required keys for changes.
    for (const auto& key : kRestartKeys) {
        auto new_val = snapshot_value(cfg, key);
        if (old_snapshots[key] != new_val)
            result.restart_warnings.push_back(key + ": " + old_snapshots[key] + " -> " + new_val);
    }

    if (content_loaded)
        result.reloaded.push_back("Re-read content config (" + ctx.content_dir + ")");

    // Snapshot old content lists for diffing.
    auto old_characters = ctx.room.characters;
    auto old_music = ctx.room.music;
    auto old_areas = ctx.room.areas;

    result.content_changed = content_loaded && (new_content.characters != old_characters ||
                                                new_content.music != old_music || new_content.area_names != old_areas);

    // -----------------------------------------------------------------------
    // Phase 3: Apply (inside dispatch lock)
    // -----------------------------------------------------------------------
    lock_wrapper([&] {
        auto& room = ctx.room;

        // --- Room fields ---
        room.server_name = cfg.server_name();
        room.server_description = cfg.server_description();
        room.max_players = cfg.max_players();
        room.max_ic_message_length = cfg.max_ic_message_length();
        room.max_ooc_message_length = cfg.max_ooc_message_length();
        room.asset_url = cfg.asset_url();
        room.mod_password = cfg.mod_password();

        // --- NX backend ---
        ctx.nx_backend.set_motd(cfg.motd());
        ctx.nx_backend.set_session_ttl_seconds(cfg.session_ttl_seconds());

        // --- Subsystem reconfiguration (all are safe to call at runtime) ---
        if (auto* rep = room.reputation_service())
            rep->configure(cfg.reputation_config());
        if (auto* asn = room.asn_reputation())
            asn->configure(cfg.asn_reputation_config());
        if (auto* sd = room.spam_detector())
            sd->configure(cfg.spam_detection_config());
        if (auto* cm = room.content_moderator())
            cm->configure(cfg.content_moderation_config());
        if (auto* fw = room.firewall())
            fw->configure(cfg.firewall_config());

        result.reloaded.push_back("Reconfigured subsystems");

        // --- Rate limiter ---
        if (ctx.rate_limiter) {
            auto rl_cfg = cfg.rate_limit_config();
            int count = 0;
            for (auto& [action, params] : rl_cfg.items()) {
                if (!params.is_object() || !params.contains("rate"))
                    continue;
                ctx.rate_limiter->configure(action, {params.value("rate", 10.0), params.value("burst", 20.0)});
                ++count;
            }
            result.reloaded.push_back("Rate limiter (" + std::to_string(count) + " rules)");

            // Update WS timeouts from rate_limit_config.
            if (ctx.ws) {
                WebSocketServer::TimeoutConfig wst;
                wst.handshake_sec = rl_cfg.value("ws_handshake_deadline_sec", 10);
                wst.idle_sec = rl_cfg.value("ws_idle_timeout_sec", 120);
                wst.partial_frame_sec = rl_cfg.value("ws_partial_frame_timeout_sec", 30);
                ctx.ws->set_timeouts(wst);
            }
        }

        // --- Content changes ---
        if (result.content_changed) {
            apply_content_changes(room, new_content, old_characters, old_areas, result);

            auto char_delta = static_cast<int>(new_content.characters.size()) - static_cast<int>(old_characters.size());
            auto area_delta = static_cast<int>(new_content.area_names.size()) - static_cast<int>(old_areas.size());
            auto music_delta = static_cast<int>(new_content.music.size()) - static_cast<int>(old_music.size());

            auto fmt_delta = [](int d) -> std::string {
                if (d > 0)
                    return " (+" + std::to_string(d) + ")";
                if (d < 0)
                    return " (" + std::to_string(d) + ")";
                return " (reordered)";
            };

            if (new_content.characters != old_characters)
                result.reloaded.push_back("Characters: " + std::to_string(old_characters.size()) + " -> " +
                                          std::to_string(new_content.characters.size()) + fmt_delta(char_delta));
            if (new_content.area_names != old_areas)
                result.reloaded.push_back("Areas: " + std::to_string(old_areas.size()) + " -> " +
                                          std::to_string(new_content.area_names.size()) + fmt_delta(area_delta));
            if (new_content.music != old_music)
                result.reloaded.push_back("Music: " + std::to_string(old_music.size()) + " -> " +
                                          std::to_string(new_content.music.size()) + fmt_delta(music_delta));
        }
    });

    // --- CORS (mutex-protected internally, safe outside dispatch lock) ---
    ctx.rest_router.set_cors_origins(cfg.cors_origins());
    if (ctx.ws)
        ctx.ws->set_cors_origins(cfg.cors_origins());

    Log::log_print(INFO, "Config reload complete (%zu changes, %zu restart-required)", result.reloaded.size(),
                   result.restart_warnings.size());
    return result;
}
