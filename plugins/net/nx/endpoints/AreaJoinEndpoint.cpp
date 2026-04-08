#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"

namespace {

class AreaJoinEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/areas/:area_id/join";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        if (rate_limiter() && !rate_limiter()->allow("nx:area_join", req.session->ipid))
            return RestResponse::error(429, "Too many area join requests");

        auto it = req.path_params.find("area_id");
        if (it == req.path_params.end())
            return RestResponse::error(400, "Missing area_id");

        const auto& area_id = it->second;

        // The spec requires a specific area id — no sentinels.
        if (area_id == "_" || area_id == "*")
            return RestResponse::error(400, "A specific area id is required");

        auto* state = room().find_area(area_id);
        if (!state)
            return RestResponse::error(404, "Area not found");

        if (!state->can_enter(req.session->client_id))
            return RestResponse::error(403, "Area is locked");

        req.session->area = state->name;

        // TODO: emit presence_update events to old and new areas (Phase 5 SSE)

        return RestResponse::json(200, {{"accepted", true}});
    }
};

EndpointRegistrar reg("POST /aonx/v1/areas/:area_id/join", [] { return std::make_unique<AreaJoinEndpoint>(); });

} // namespace

void nx_ep_area_join() {
}
