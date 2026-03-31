#include "net/nx/NXEndpoint.h"

#include "AreaIdResolver.h"
#include "net/EndpointRegistrar.h"

namespace {

class AreaPlayersEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/areas/:area_id/players";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        auto it = req.path_params.find("area_id");
        if (it == req.path_params.end())
            return RestResponse::error(400, "Missing area_id");

        auto* state = resolve_area(it->second, req.session, room());
        if (!state)
            return RestResponse::error(404, "Area not found");

        auto sessions = room().sessions_in_area(state->name);

        nlohmann::json players = nlohmann::json::array();
        for (const auto* s : sessions) {
            nlohmann::json character = nullptr;
            if (s->character_id >= 0)
                character = room().char_id_at(s->character_id);

            players.push_back({
                {"user_id", std::to_string(s->session_id)},
                {"display_name", s->display_name},
                {"character", std::move(character)},
            });
        }

        return RestResponse::json(200, {{"players", std::move(players)}});
    }
};

EndpointRegistrar reg("GET /aonx/v1/areas/:area_id/players", [] { return std::make_unique<AreaPlayersEndpoint>(); });

} // namespace

void nx_ep_area_players() {
}
