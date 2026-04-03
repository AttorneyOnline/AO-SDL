#include "net/EndpointRegistrar.h"
#include "net/nx/NXEndpoint.h"
#include "net/nx/NXTime.h"

#include <chrono>

namespace {

class SessionRenewEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "PATCH";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/session";
        return p;
    }
    bool requires_auth() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        // touch() is already called by RestRouter for authenticated requests,
        // but call it explicitly to make the intent clear.
        req.session->touch();

        int ttl = server().session_ttl_seconds();
        nlohmann::json body = {
            {"token", req.session->session_token},
        };

        if (ttl > 0) {
            auto expires_at = std::chrono::system_clock::now() + std::chrono::seconds(ttl);
            body["expires_at"] = to_iso8601(expires_at);
        }

        return RestResponse::json(200, std::move(body));
    }
};

EndpointRegistrar reg("PATCH /aonx/v1/session", [] { return std::make_unique<SessionRenewEndpoint>(); });

} // namespace

// Linker anchor — referenced by nx_register_endpoints().
void nx_ep_session_renew() {
}
