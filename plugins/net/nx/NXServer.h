#pragma once

#include "game/GameAction.h"
#include "game/GameRoom.h"

#include <atomic>
#include <cstdint>
#include <string>

class NXServer {
  public:
    explicit NXServer(GameRoom& room);

    GameRoom& room() {
        return room_;
    }

    /// Create a session for a REST client. Returns the bearer token.
    std::string create_session(const std::string& hdid, const std::string& client_name,
                               const std::string& client_version);
    void destroy_session(uint64_t client_id);

  private:
    // Broadcast callbacks registered with GameRoom.
    // These will push events to SSE streams once implemented (Phase 5).
    void broadcast_ic(const std::string& area, const ICEvent& evt);
    void broadcast_ooc(const std::string& area, const OOCEvent& evt);
    void broadcast_char_select(const CharSelectEvent& evt);
    void broadcast_chars_taken(const std::vector<int>& taken);

    GameRoom& room_;
    std::atomic<uint64_t> next_rest_id_ =
        0x8000'0000'0000'0000ULL; ///< High bit set to avoid collision with WS client IDs.
};
