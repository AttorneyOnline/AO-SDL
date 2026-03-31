#include "net/nx/NXEndpoint.h"

#include "AreaIdResolver.h"
#include "net/EndpointRegistrar.h"

namespace {

class AreaGetEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/areas/:area_id";
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

        nlohmann::json area_obj = {
            {"id", state->id},         {"name", state->name},
            {"path", state->path},     {"players", static_cast<int>(room().sessions_in_area(state->name).size())},
            {"status", state->status}, {"cm", state->cm},
            {"locked", state->locked},
        };

        // Background and music are nullable — return null if empty.
        nlohmann::json bg = nullptr;
        if (!state->background.name.empty()) {
            bg = {
                {"name", state->background.name},
                {"manifest_hash", state->background.manifest_hash},
                {"position", state->background.position},
            };
        }

        nlohmann::json mus = nullptr;
        if (!state->music.name.empty()) {
            mus = {
                {"name", state->music.name},
                {"asset_hash", state->music.asset_hash},
                {"looping", state->music.looping},
                {"channel", state->music.channel},
            };
        }

        nlohmann::json timers = nlohmann::json::array();
        for (const auto& [tid, t] : state->timers) {
            timers.push_back({
                {"timer_id", tid},
                {"value_ms", t.value_ms},
                {"running", t.running},
            });
        }

        return RestResponse::json(200,
                                  {
                                      {"area", std::move(area_obj)},
                                      {"background", std::move(bg)},
                                      {"music", std::move(mus)},
                                      {"hp", {{"defense", state->hp.defense}, {"prosecution", state->hp.prosecution}}},
                                      {"timers", std::move(timers)},
                                  });
    }
};

EndpointRegistrar reg("GET /aonx/v1/areas/:area_id", [] { return std::make_unique<AreaGetEndpoint>(); });

} // namespace

void nx_ep_area_get() {
}
