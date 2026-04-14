#include "net/nx/NXEndpoint.h"

#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointRegistrar.h"
#include "net/SSEEvent.h"
#include "utils/Log.h"

#include "event/EventManager.h"

#include <json.hpp>

namespace {

/// Send a session_ended SSE event to a specific session token, then
/// destroy all sessions that were created with the given auth token.
void kill_sessions_for_token(GameRoom& room, const std::string& auth_token) {
    std::vector<uint64_t> to_destroy;
    room.for_each_session([&](const ServerSession& s) {
        if (s.auth_token_id == auth_token)
            to_destroy.push_back(s.client_id);
    });

    for (auto id : to_destroy) {
        // Send targeted SSE notification before destroying
        auto* session = room.get_session(id);
        if (session && !session->session_token.empty()) {
            SSEEvent evt;
            evt.event = "session_ended";
            evt.data = R"({"reason":"Your auth token was revoked. Please log in again."})";
            evt.target_token = session->session_token;
            EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));
        }
        room.destroy_session(id);
    }

    if (!to_destroy.empty())
        Log::log_print(INFO, "Auth: killed %zu session(s) for revoked auth token", to_destroy.size());
}

class AuthLogoutEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/auth/logout";
        return p;
    }
    bool requires_auth() const override {
        // We handle auth manually -- the Bearer token here is an auth token,
        // not a session token, so the standard session auth check won't work.
        return false;
    }
    // NOT lock_free — kill_sessions_for_token modifies session state.

    RestResponse handle(const RestRequest& req) override {
        if (req.bearer_token.empty())
            return RestResponse::error(401, "Missing auth token in Authorization header");

        auto* db = room().db_manager();
        if (!db || !db->is_open())
            return RestResponse::error(503, "Database unavailable");

        // Validate the token exists (even if revoked, we return success for idempotency)
        bool revoked = db->revoke_auth_token(req.bearer_token).get();
        if (!revoked) {
            // Token not found or already revoked
            return RestResponse::error(401, "Invalid or already revoked auth token");
        }

        // Kill active sessions bound to this token
        kill_sessions_for_token(room(), req.bearer_token);

        Log::log_print(INFO, "Auth: token revoked via REST logout");
        return RestResponse::json(200, {{"revoked", true}});
    }
};

EndpointRegistrar reg("POST /aonx/v1/auth/logout", [] { return std::make_unique<AuthLogoutEndpoint>(); });

} // namespace

void nx_ep_auth_logout() {
}
