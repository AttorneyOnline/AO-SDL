#include "net/nx/NXEndpoint.h"

#include "game/ACLFlags.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"
#include "net/EndpointRegistrar.h"
#include "utils/Log.h"

#include <json.hpp>

namespace {

class AdminStopEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/stop";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }
    CorsPolicy cors_policy() const override {
        return CorsPolicy::Restricted;
    }

    RestResponse handle(const RestRequest& req) override {
        if (!req.session || !req.session->moderator)
            return RestResponse::error(403, "Authentication required");
        if (!has_permission(acl_permissions_for_role(req.session->acl_role), ACLPermission::SUPER))
            return RestResponse::error(403, "SUPER privileges required");

        const auto& stop = room().stop_func();
        if (!stop)
            return RestResponse::error(503, "Stop not available");

        Log::log_print(INFO, "Admin: server stop requested by %s via REST", req.session->moderator_name.c_str());

        // Send response before stopping so the client gets the 200
        auto response = RestResponse::json(200, {{"status", "stopping"}});

        // Request stop on a detached thread so the HTTP response finishes first
        std::thread([stop]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            stop();
        }).detach();

        return response;
    }
};

EndpointRegistrar reg("POST /aonx/v1/admin/stop", [] { return std::make_unique<AdminStopEndpoint>(); });

} // namespace

void nx_ep_admin_stop() {
}
