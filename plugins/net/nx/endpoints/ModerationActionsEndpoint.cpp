#include "net/nx/NXEndpoint.h"
#include "net/nx/NXTime.h"

#include "game/BanManager.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointRegistrar.h"
#include "net/SSEEvent.h"
#include "utils/Crypto.h"
#include "utils/GeneratedSchemas.h"
#include "utils/Log.h"

#include "event/EventManager.h"

#include <json.hpp>

#include <chrono>

namespace {

auto now_sec() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

/// POST /moderation/actions
/// General-purpose moderation endpoint. Dispatches kick, ban, unban,
/// mute, unmute, and notice (global broadcast) to the appropriate
/// subsystem. Custom x- prefixed actions are accepted but no-op'd.
class ModerationActionsEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/moderation/actions";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        if (!req.session || !req.session->moderator)
            return RestResponse::error(403, "Moderator privileges required");

        if (!req.body)
            return RestResponse::error(400, "Request body is required");

        if (auto err = aonx_request_schema("submitModerationAction").validate(*req.body); !err.empty())
            return RestResponse::error(400, err);

        auto action = req.body->value("action", std::string{});
        auto target = req.body->value("target", std::string{});
        auto reason = req.body->value("reason", std::string{"No reason given"});
        auto duration = req.body->value("duration", -1); // -1 = permanent

        auto action_id = crypto::sha256(action + ":" + target + ":" + std::to_string(now_sec())).substr(0, 16);

        if (action == "kick")
            return do_kick(req, target, reason, action_id);
        if (action == "ban")
            return do_ban(req, target, reason, duration, action_id);
        if (action == "unban")
            return do_unban(target, action_id);
        if (action == "mute")
            return do_mute(target, reason, duration, action_id);
        if (action == "unmute")
            return do_unmute(target, action_id);
        if (action == "notice")
            return do_notice(req, reason, action_id);

        // Custom x- actions: accept but don't do anything
        if (action.starts_with("x-"))
            return success_response(action, action_id);

        return RestResponse::error(400, "Unknown action: " + action);
    }

  private:
    RestResponse success_response(const std::string& action, const std::string& action_id) {
        return RestResponse::json(200, {
                                           {"action_id", action_id},
                                           {"action", action},
                                           {"status", "applied"},
                                           {"timestamp", to_iso8601(std::chrono::system_clock::now())},
                                       });
    }

    RestResponse do_kick(const RestRequest& req, const std::string& target, const std::string& reason,
                         const std::string& action_id) {
        if (target.empty())
            return RestResponse::error(400, "kick requires a target (IPID or session_id)");

        int kicked = 0;
        room().for_each_session([&](const ServerSession& s) {
            if (s.ipid == target || std::to_string(s.session_id) == target) {
                // Send SSE notification if REST session
                if (!s.session_token.empty()) {
                    SSEEvent evt;
                    evt.event = "session_ended";
                    evt.data = nlohmann::json({{"reason", "Kicked: " + reason}}).dump();
                    evt.target_token = s.session_token;
                    EventManager::instance().get_channel<SSEEvent>().publish(std::move(evt));
                }
                ++kicked;
            }
        });

        // Destroy sessions (separate pass to avoid mutating during iteration)
        std::vector<uint64_t> to_destroy;
        room().for_each_session([&](const ServerSession& s) {
            if (s.ipid == target || std::to_string(s.session_id) == target)
                to_destroy.push_back(s.client_id);
        });
        for (auto id : to_destroy)
            room().destroy_session(id);

        Log::log_print(INFO, "Moderation: %s kicked %s (%s) — %d session(s)", req.session->moderator_name.c_str(),
                       target.c_str(), reason.c_str(), kicked);
        return success_response("kick", action_id);
    }

    RestResponse do_ban(const RestRequest& req, const std::string& target, const std::string& reason, int duration,
                        const std::string& action_id) {
        if (target.empty())
            return RestResponse::error(400, "ban requires a target (IPID)");

        auto* bm = room().ban_manager();
        if (!bm)
            return RestResponse::error(503, "Ban manager unavailable");

        // Look up IP from connected sessions for firewall integration
        std::string ip;
        room().for_each_session([&](const ServerSession& s) {
            if (s.ipid == target && !s.ip_address.empty())
                ip = s.ip_address;
        });

        BanEntry entry;
        entry.ipid = target;
        entry.ip = ip;
        entry.reason = reason;
        entry.moderator = req.session->moderator_name.empty() ? req.session->display_name
                                                              : req.session->moderator_name;
        entry.timestamp = now_sec();
        entry.duration = duration < 0 ? -2 : duration; // -2 = permanent
        bm->add_ban(std::move(entry));

        // Kick the banned user
        do_kick(req, target, "Banned: " + reason, action_id);

        Log::log_print(INFO, "Moderation: %s banned %s for %ds (%s)", req.session->moderator_name.c_str(),
                       target.c_str(), duration, reason.c_str());
        return success_response("ban", action_id);
    }

    RestResponse do_unban(const std::string& target, const std::string& action_id) {
        if (target.empty())
            return RestResponse::error(400, "unban requires a target (IPID or ban ID)");

        auto* bm = room().ban_manager();
        if (!bm)
            return RestResponse::error(503, "Ban manager unavailable");

        if (!bm->remove_ban(target))
            return RestResponse::error(404, "Ban not found for IPID: " + target);

        Log::log_print(INFO, "Moderation: unbanned %s", target.c_str());
        return success_response("unban", action_id);
    }

    RestResponse do_mute(const std::string& target, const std::string& reason, int duration,
                         const std::string& action_id) {
        if (target.empty())
            return RestResponse::error(400, "mute requires a target (IPID)");

        auto* db = room().db_manager();
        if (!db || !db->is_open())
            return RestResponse::error(503, "Database unavailable");

        MuteEntry mute;
        mute.ipid = target;
        mute.started_at = now_sec();
        mute.expires_at = duration > 0 ? mute.started_at + duration : 0;
        mute.reason = reason;
        mute.moderator = "REST API";
        db->add_mute(std::move(mute));

        Log::log_print(INFO, "Moderation: muted %s for %ds (%s)", target.c_str(), duration, reason.c_str());
        return success_response("mute", action_id);
    }

    RestResponse do_unmute(const std::string& target, const std::string& action_id) {
        if (target.empty())
            return RestResponse::error(400, "unmute requires a target (IPID)");

        auto* db = room().db_manager();
        if (!db || !db->is_open())
            return RestResponse::error(503, "Database unavailable");

        int removed = db->delete_mutes_by_ipid(target).get();
        if (removed == 0)
            return RestResponse::error(404, "No active mute for: " + target);

        Log::log_print(INFO, "Moderation: unmuted %s (%d mute(s) removed)", target.c_str(), removed);
        return success_response("unmute", action_id);
    }

    RestResponse do_notice(const RestRequest& req, const std::string& message, const std::string& action_id) {
        if (message.empty())
            return RestResponse::error(400, "notice requires a reason (the message text)");

        // Broadcast as OOC to all areas
        OOCAction ooc;
        ooc.sender_id = req.session->client_id;
        ooc.name = "[Server]";
        ooc.message = message;
        room().handle_ooc(ooc);

        Log::log_print(INFO, "Moderation: global notice from %s: %s", req.session->moderator_name.c_str(),
                       message.c_str());
        return success_response("notice", action_id);
    }
};

EndpointRegistrar reg("POST /aonx/v1/moderation/actions", [] { return std::make_unique<ModerationActionsEndpoint>(); });

} // namespace

void nx_ep_moderation_actions() {
}
