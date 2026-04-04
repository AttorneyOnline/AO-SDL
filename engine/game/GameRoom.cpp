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
    auto session = std::make_shared<ServerSession>();
    session->client_id = client_id;
    session->session_id = next_session_id_++;
    session->protocol = protocol;
    if (!areas.empty())
        session->area = areas[0];

    ServerSession* raw = session.get();
    cow_mutate([&](SessionMap& sm, TokenMap&) { sm[client_id] = std::move(session); });

    (protocol == "ao2") ? stats.sessions_ao.fetch_add(1, std::memory_order_relaxed)
                        : stats.sessions_nx.fetch_add(1, std::memory_order_relaxed);
    Log::log_print(INFO, "GameRoom: session created for %s", format_client_id(client_id).c_str());
    return *raw;
}

void GameRoom::destroy_session(uint64_t client_id) {
    auto it = sessions_->find(client_id);
    if (it == sessions_->end())
        return;

    auto& session = *it->second;

    // Update atomic stats before erasing
    (session.protocol == "ao2") ? stats.sessions_ao.fetch_sub(1, std::memory_order_relaxed)
                                : stats.sessions_nx.fetch_sub(1, std::memory_order_relaxed);
    if (session.joined)
        stats.joined.fetch_sub(1, std::memory_order_relaxed);
    if (session.moderator)
        stats.moderators.fetch_sub(1, std::memory_order_relaxed);

    // Free character slot
    int char_id = session.character_id;
    if (char_id >= 0 && char_id < static_cast<int>(char_taken.size())) {
        char_taken[char_id] = 0;
        stats.chars_taken.fetch_sub(1, std::memory_order_relaxed);
    }

    cow_mutate([&](SessionMap& sm, TokenMap& tm) {
        if (!session.session_token.empty())
            tm.erase(session.session_token);
        sm.erase(client_id);
    });

    Log::log_print(INFO, "GameRoom: session destroyed for %s", format_client_id(client_id).c_str());
}

ServerSession* GameRoom::get_session(uint64_t client_id) {
    auto it = sessions_->find(client_id);
    return it != sessions_->end() ? it->second.get() : nullptr;
}

size_t GameRoom::session_count() const {
    return sessions_->size();
}

void GameRoom::register_session_token(const std::string& token, uint64_t client_id) {
    if (!token.empty())
        cow_mutate([&](SessionMap&, TokenMap& tm) { tm[token] = client_id; });
}

ServerSession* GameRoom::find_session_by_token(const std::string& token) {
    auto it = token_index_->find(token);
    if (it == token_index_->end())
        return nullptr;
    return get_session(it->second);
}

int GameRoom::expire_sessions(int ttl_seconds) {
    if (ttl_seconds <= 0)
        return 0;
    auto now = std::chrono::steady_clock::now();
    int expired = 0;

    // Collect expired IDs first (can't modify during COW iteration)
    std::vector<uint64_t> to_expire;
    for (auto& [client_id, session] : *sessions_) {
        if (!session->session_token.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->last_activity()).count();
            if (elapsed > ttl_seconds)
                to_expire.push_back(client_id);
        }
    }

    for (auto cid : to_expire) {
        destroy_session(cid);
        ++expired;
    }

    if (expired > 0) {
        sessions_expired_.get().inc(expired);
        for (auto& cb : chars_taken_broadcasts_)
            cb(char_taken);
    }
    return expired;
}

std::vector<uint64_t> GameRoom::find_expired_sessions(int ttl_seconds) const {
    std::vector<uint64_t> result;
    if (ttl_seconds <= 0)
        return result;
    auto now = std::chrono::steady_clock::now();
    for (auto& [client_id, session] : *sessions_) {
        if (!session->session_token.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session->last_activity()).count();
            if (elapsed > ttl_seconds)
                result.push_back(client_id);
        }
    }
    return result;
}

GameRoom::SessionSnapshot GameRoom::sessions_snapshot() const {
    return {sessions_, token_index_};
}

std::vector<ServerSession*> GameRoom::sessions_in_area(const std::string& area) {
    std::vector<ServerSession*> result;
    for (auto& [id, s] : *sessions_) {
        if (s->area == area)
            result.push_back(s.get());
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
    if (old_char >= 0 && old_char < static_cast<int>(char_taken.size())) {
        char_taken[old_char] = 0;
        stats.chars_taken.fetch_sub(1, std::memory_order_relaxed);
    }

    // Take new (if not spectator)
    if (requested >= 0) {
        if (char_taken[requested]) {
            Log::log_print(INFO, "GameRoom: char %d already taken", requested);
            return false;
        }
        char_taken[requested] = 1;
        stats.chars_taken.fetch_add(1, std::memory_order_relaxed);
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
