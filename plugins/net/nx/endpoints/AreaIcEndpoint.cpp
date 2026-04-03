#include "net/nx/NXEndpoint.h"

#include "AreaIdResolver.h"
#include "net/EndpointRegistrar.h"
#include "utils/GeneratedSchemas.h"

#include <cassert>

namespace {

/// Flatten an NX IcMessage JSON into the protocol-agnostic ICAction struct.
///
/// The IcMessage composition model is richer than what AO2 can represent.
/// This function extracts the primary entry from each array (text[0],
/// objects[0], audio[0]) and maps it to the flat ICAction fields that
/// GameRoom::handle_ic() broadcasts to all backends.
///
/// Full NX-to-NX forwarding of the raw composition will be added in Phase 5
/// when SSE event streams are implemented.
ICAction flatten_ic_message(const nlohmann::json& body, ServerSession* session) {
    assert(session);
    ICAction action;
    action.sender_id = session->client_id;
    action.char_id = session->character_id;

    // Character name from session (set by character select).
    if (action.char_id >= 0)
        action.character = session->display_name;

    // -- Text --
    if (body.contains("text") && body["text"].is_array() && !body["text"].empty()) {
        auto& t = body["text"][0];
        action.message = t.value("content", std::string{});
        action.showname = t.value("showname", std::string{});
        action.additive = t.value("additive", false);
        action.immediate = t.value("immediate", false);
        // TODO: map hex color string to AO2 color index (0=white for now)
    }

    // -- Objects (character sprite / positioning) --
    if (body.contains("objects") && body["objects"].is_array() && !body["objects"].empty()) {
        auto& obj = body["objects"][0];

        // The object id often maps to the courtroom side (e.g. "def", "wit", "pro").
        action.side = obj.value("id", std::string{});

        // initial_state maps to the emote/animation name.
        action.emote = obj.value("initial_state", std::string{});

        // ref can encode character layer (e.g. "def", "def:desk").
        if (action.emote.empty() && obj.contains("ref"))
            action.emote = obj.value("ref", std::string{});

        // Flip detection: negative scale_x means horizontally flipped.
        if (obj.contains("transform") && obj["transform"].is_object()) {
            auto scale_x = obj["transform"].value("scale_x", 1.0);
            action.flip = scale_x < 0;
        }
    }

    // -- Audio (SFX) --
    if (body.contains("audio") && body["audio"].is_array() && !body["audio"].empty()) {
        auto& sfx = body["audio"][0];
        action.sfx_name = sfx.value("asset", std::string{});
        action.sfx_delay = sfx.value("delay_ms", 0);
    }

    // -- Effects --
    // Map known shader names to AO2's boolean effect flags.
    if (body.contains("effects") && body["effects"].is_array()) {
        for (auto& eff : body["effects"]) {
            auto shader = eff.value("shader", std::string{});
            if (shader.starts_with("realization"))
                action.realization = true;
            else if (shader.starts_with("screenshake"))
                action.screenshake = true;
        }
    }

    return action;
}

class AreaIcEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/areas/:area_id/ic";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    bool readonly() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        auto it = req.path_params.find("area_id");
        if (it == req.path_params.end())
            return RestResponse::error(400, "Missing area_id");

        if (!req.body)
            return RestResponse::error(400, "Request body is required");

        auto& body = *req.body;

        if (auto err = aonx_request_schema("sendIc").validate(body); !err.empty())
            return RestResponse::error(400, err);

        auto action = flatten_ic_message(body, req.session);

        const auto& area_id = it->second;

        if (area_id == "*") {
            if (!req.session->moderator)
                return RestResponse::error(403, "All-areas broadcast requires moderator");
            for (auto& [id, state] : room().area_states())
                room().handle_ic(action, state.name);
        }
        else {
            auto* state = resolve_area(area_id, req.session, room());
            if (!state)
                return RestResponse::error(404, "Area not found");
            room().handle_ic(action, state->name);
        }

        return RestResponse::json(200, {{"accepted", true}});
    }
};

EndpointRegistrar reg("POST /aonx/v1/areas/:area_id/ic", [] { return std::make_unique<AreaIcEndpoint>(); });

} // namespace

void nx_ep_area_ic() {
}
