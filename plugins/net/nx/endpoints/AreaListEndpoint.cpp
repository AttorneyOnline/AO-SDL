#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"

namespace {

class AreaListEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/areas";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& /*req*/) override {
        nlohmann::json list = nlohmann::json::array();

        for (const auto& [id, state] : room().area_states()) {
            list.push_back({
                {"id", state.id},
                {"name", state.name},
                {"path", state.path},
                {"players", static_cast<int>(room().sessions_in_area(state.name).size())},
                {"status", state.status},
                {"cm", state.cm},
                {"locked", state.locked},
            });
        }

        return RestResponse::json(200, {{"areas", std::move(list)}});
    }
};

EndpointRegistrar reg("GET /aonx/v1/areas", [] { return std::make_unique<AreaListEndpoint>(); });

} // namespace

void nx_ep_area_list() {
}
