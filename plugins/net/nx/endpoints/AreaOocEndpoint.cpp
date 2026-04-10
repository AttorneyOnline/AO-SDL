#include "net/nx/NXEndpoint.h"

#include "AreaIdResolver.h"
#include "moderation/ContentModerator.h"
#include "net/EndpointRegistrar.h"
#include "utils/GeneratedSchemas.h"

namespace {

class AreaOocEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/areas/:area_id/ooc";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        if (rate_limiter() && !rate_limiter()->allow("nx:ooc", req.session->ipid))
            return RestResponse::error(429, "Too many OOC messages");

        auto it = req.path_params.find("area_id");
        if (it == req.path_params.end())
            return RestResponse::error(400, "Missing area_id");

        if (!req.body)
            return RestResponse::error(400, "Request body is required");

        auto& body = *req.body;

        if (auto err = aonx_request_schema("sendOoc").validate(body); !err.empty())
            return RestResponse::error(400, err);

        OOCAction action;
        action.sender_id = req.session->client_id;
        action.name = body.value("name", std::string{});
        action.message = body.value("message", std::string{});

        int max_ooc = room().max_ooc_message_length;
        if (max_ooc > 0 && static_cast<int>(action.message.size()) > max_ooc)
            return RestResponse::error(400, "OOC message too long");

        // Content moderation — mirrors the AO2 packet handler path.
        // Returns an HTTP error for non-broadcast outcomes so the
        // client sees the same semantic signal as an AO2 client does
        // via suppressed broadcast.
        if (auto* cm = room().content_moderator()) {
            // No eager is_muted check — clean messages pass through even
            // for muted users under the per-message filtering rule.
            auto v = cm->check(req.session->ipid, "ooc", action.message);
            switch (v.action) {
            case moderation::ModerationAction::NONE:
            case moderation::ModerationAction::LOG:
                break;
            case moderation::ModerationAction::CENSOR:
                action.message = "[filtered]";
                break;
            case moderation::ModerationAction::DROP:
            case moderation::ModerationAction::MUTE:
                return RestResponse::json(200, {{"accepted", false}, {"reason", "content"}});
            case moderation::ModerationAction::KICK:
            case moderation::ModerationAction::BAN:
            case moderation::ModerationAction::PERMA_BAN:
                return RestResponse::error(403, "Content moderation: " + v.reason);
            }
        }

        const auto& area_id = it->second;

        if (area_id == "*") {
            if (!req.session->moderator)
                return RestResponse::error(403, "All-areas broadcast requires moderator");
            for (auto& [id, state] : room().area_states())
                room().handle_ooc(action, state.name);
        }
        else {
            auto* state = resolve_area(area_id, req.session, room());
            if (!state)
                return RestResponse::error(404, "Area not found");
            room().handle_ooc(action, state->name);
        }

        return RestResponse::json(200, {{"accepted", true}});
    }
};

EndpointRegistrar reg("POST /aonx/v1/areas/:area_id/ooc", [] { return std::make_unique<AreaOocEndpoint>(); });

} // namespace

void nx_ep_area_ooc() {
}
