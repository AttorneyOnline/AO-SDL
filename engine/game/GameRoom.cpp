#include "game/GameRoom.h"

#include "game/ClientId.h"
#include "metrics/MetricsRegistry.h"
#include "utils/Crypto.h"
#include "utils/Log.h"

static auto& sessions_expired_ =
    metrics::MetricsRegistry::instance().counter("kagami_sessions_expired_total", "Sessions expired by TTL");

#include <algorithm>
#include <cctype>

ServerSession& GameRoom::create_session(uint64_t client_id, const std::string& protocol) {
    ServerSession session;
    session.client_id = client_id;
    session.session_id = next_session_id_++;
    session.protocol = protocol;
    if (!areas.empty())
        session.area = areas[0];

    auto [it, _] = sessions_.emplace(client_id, std::move(session));
    Log::log_print(INFO, "GameRoom: session created for %s", format_client_id(client_id).c_str());
    return it->second;
}

void GameRoom::destroy_session(uint64_t client_id) {
    auto it = sessions_.find(client_id);
    if (it == sessions_.end())
        return;

    // Free character slot
    int char_id = it->second.character_id;
    if (char_id >= 0 && char_id < static_cast<int>(char_taken.size()))
        char_taken[char_id] = 0;

    if (!it->second.session_token.empty())
        token_index_.erase(it->second.session_token);

    Log::log_print(INFO, "GameRoom: session destroyed for %s", format_client_id(client_id).c_str());
    sessions_.erase(it);
}

ServerSession* GameRoom::get_session(uint64_t client_id) {
    auto it = sessions_.find(client_id);
    return it != sessions_.end() ? &it->second : nullptr;
}

void GameRoom::register_session_token(const std::string& token, uint64_t client_id) {
    if (!token.empty())
        token_index_[token] = client_id;
}

ServerSession* GameRoom::find_session_by_token(const std::string& token) {
    auto it = token_index_.find(token);
    if (it == token_index_.end())
        return nullptr;
    return get_session(it->second);
}

int GameRoom::expire_sessions(int ttl_seconds) {
    if (ttl_seconds <= 0)
        return 0;
    auto now = std::chrono::steady_clock::now();
    int expired = 0;
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        auto& session = it->second;
        // Only expire REST sessions (those with tokens). AO2/WS sessions
        // are managed by their transport layer.
        if (!session.session_token.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session.last_activity()).count();
            if (elapsed > ttl_seconds) {
                // Free character slot
                if (session.character_id >= 0 && session.character_id < static_cast<int>(char_taken.size()))
                    char_taken[session.character_id] = 0;
                token_index_.erase(session.session_token);
                Log::log_print(INFO, "GameRoom: expired %s (inactive %llds)", format_client_id(it->first).c_str(),
                               (long long)elapsed);
                it = sessions_.erase(it);
                ++expired;
                continue;
            }
        }
        ++it;
    }
    if (expired > 0) {
        sessions_expired_.get().inc(expired);
        for (auto& cb : chars_taken_broadcasts_)
            cb(char_taken);
    }
    return expired;
}

std::vector<ServerSession*> GameRoom::sessions_in_area(const std::string& area) {
    std::vector<ServerSession*> result;
    for (auto& [id, s] : sessions_) {
        if (s.area == area)
            result.push_back(&s);
    }
    return result;
}

void GameRoom::handle_ic(const ICAction& action) {
    auto* session = get_session(action.sender_id);
    if (!session || !session->joined)
        return;

    Log::log_print(INFO, "GameRoom: IC from %s in %s", session->display_name.c_str(), session->area.c_str());

    ICEvent evt{session->area, action};
    for (auto& cb : ic_broadcasts_)
        cb(evt.area, evt);
}

void GameRoom::handle_ic(const ICAction& action, const std::string& target_area) {
    auto* session = get_session(action.sender_id);
    if (!session || !session->joined)
        return;

    Log::log_print(INFO, "GameRoom: IC from %s -> %s", session->display_name.c_str(), target_area.c_str());

    ICEvent evt{target_area, action};
    for (auto& cb : ic_broadcasts_)
        cb(evt.area, evt);
}

void GameRoom::handle_ooc(const OOCAction& action) {
    auto* session = get_session(action.sender_id);
    if (!session || !session->joined)
        return;

    Log::log_print(INFO, "GameRoom: OOC from %s in %s", action.name.c_str(), session->area.c_str());

    OOCEvent evt{session->area, action};
    for (auto& cb : ooc_broadcasts_)
        cb(evt.area, evt);
}

void GameRoom::handle_ooc(const OOCAction& action, const std::string& target_area) {
    auto* session = get_session(action.sender_id);
    if (!session || !session->joined)
        return;

    Log::log_print(INFO, "GameRoom: OOC from %s -> %s", action.name.c_str(), target_area.c_str());

    OOCEvent evt{target_area, action};
    for (auto& cb : ooc_broadcasts_)
        cb(evt.area, evt);
}

bool GameRoom::handle_char_select(const CharSelectAction& action) {
    auto* session = get_session(action.sender_id);
    if (!session)
        return false;

    int requested = action.character_id;

    // Validate range
    if (requested < -1 || requested >= static_cast<int>(characters.size()))
        return false;

    // Free previous
    int old_char = session->character_id;
    if (old_char >= 0 && old_char < static_cast<int>(char_taken.size()))
        char_taken[old_char] = 0;

    // Take new (if not spectator)
    if (requested >= 0) {
        if (char_taken[requested]) {
            Log::log_print(INFO, "GameRoom: char %d already taken", requested);
            return false;
        }
        char_taken[requested] = 1;
    }

    session->character_id = requested;
    if (requested >= 0)
        session->display_name = characters[requested];

    Log::log_print(INFO, "GameRoom: %s selected character %d (%s)", format_client_id(action.sender_id).c_str(),
                   requested, session->display_name.c_str());

    CharSelectEvent evt{action.sender_id, requested, session->display_name};
    for (auto& cb : char_select_broadcasts_)
        cb(evt);

    for (auto& cb : chars_taken_broadcasts_)
        cb(char_taken);

    return true;
}

void GameRoom::handle_music(const MusicAction& action) {
    auto* session = get_session(action.sender_id);
    if (!session || !session->joined)
        return;

    Log::log_print(INFO, "GameRoom: music '%s' from %s in %s", action.track.c_str(), session->display_name.c_str(),
                   session->area.c_str());

    MusicEvent evt{session->area, action};
    for (auto& cb : music_broadcasts_)
        cb(evt.area, evt);
}

// -- Character ID index (Phase 3) -------------------------------------------

const std::string GameRoom::empty_char_id_;

void GameRoom::build_char_id_index() {
    char_ids_.clear();
    char_id_to_index_.clear();
    char_ids_.reserve(characters.size());
    for (int i = 0; i < static_cast<int>(characters.size()); ++i) {
        auto id = crypto::sha256("character:" + characters[i]);
        char_id_to_index_[id] = i;
        char_ids_.push_back(std::move(id));
    }
}

int GameRoom::find_char_index(const std::string& char_id) const {
    auto it = char_id_to_index_.find(char_id);
    return it != char_id_to_index_.end() ? it->second : -1;
}

const std::string& GameRoom::char_id_at(int index) const {
    if (index < 0 || index >= static_cast<int>(char_ids_.size()))
        return empty_char_id_;
    return char_ids_[index];
}

// -- Area state index (Phase 3) ----------------------------------------------

/// Slugify: lowercase, spaces/underscores → hyphens, strip non-alnum.
static std::string slugify(const std::string& name) {
    std::string slug;
    slug.reserve(name.size());
    for (char c : name) {
        if (c == ' ' || c == '_')
            slug += '-';
        else if (std::isalnum(static_cast<unsigned char>(c)) || c == '-')
            slug += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return slug;
}

void GameRoom::build_area_index() {
    area_states_.clear();
    area_name_to_id_.clear();
    for (const auto& name : areas) {
        AreaState state;
        state.id = crypto::sha256("area:" + name);
        state.name = name;
        state.path = slugify(name);
        area_name_to_id_[name] = state.id;
        area_states_.emplace(state.id, std::move(state));
    }
}

AreaState* GameRoom::find_area(const std::string& area_id) {
    auto it = area_states_.find(area_id);
    return it != area_states_.end() ? &it->second : nullptr;
}

AreaState* GameRoom::find_area_by_name(const std::string& area_name) {
    auto it = area_name_to_id_.find(area_name);
    if (it == area_name_to_id_.end())
        return nullptr;
    return find_area(it->second);
}
