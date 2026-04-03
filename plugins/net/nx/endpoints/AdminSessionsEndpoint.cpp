#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"

#include <json.hpp>

#include <chrono>

namespace {

class AdminSessionsEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "GET";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/admin/sessions";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        if (!req.session || !req.session->moderator)
            return RestResponse::error(403, "Moderator privileges required");

        auto now = std::chrono::steady_clock::now();
        nlohmann::json arr = nlohmann::json::array();

        room().for_each_session([&](const ServerSession& s) {
            auto idle = std::chrono::duration_cast<std::chrono::seconds>(now - s.last_activity).count();
            arr.push_back({
                {"session_id", s.session_id},
                {"display_name", s.display_name},
                {"protocol", s.protocol},
                {"area", s.area},
                {"character_id", s.character_id},
                {"bytes_sent", s.bytes_sent.load(std::memory_order_relaxed)},
                {"bytes_received", s.bytes_received.load(std::memory_order_relaxed)},
                {"packets_sent", s.packets_sent.load(std::memory_order_relaxed)},
                {"packets_received", s.packets_received.load(std::memory_order_relaxed)},
                {"mod_actions", s.mod_actions.load(std::memory_order_relaxed)},
                {"idle_seconds", idle},
            });
        });

        return RestResponse::json(200, std::move(arr));
    }
};

EndpointRegistrar reg("GET /aonx/v1/admin/sessions", [] { return std::make_unique<AdminSessionsEndpoint>(); });

} // namespace

// Linker anchor — referenced by nx_register_endpoints().
void nx_ep_admin_sessions() {
}
