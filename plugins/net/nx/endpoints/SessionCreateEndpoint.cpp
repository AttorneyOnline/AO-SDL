#include "net/nx/NXEndpoint.h"
#include "net/nx/NXTime.h"

#include "net/EndpointRegistrar.h"
#include "utils/GeneratedSchemas.h"

#include <chrono>

namespace {

class SessionCreateEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/session";
        return p;
    }
    bool requires_auth() const override {
        return false;
    }
    bool lock_free() const override {
        return true;
    }
    bool sensitive() const override {
        return true;
    }

    RestResponse handle(const RestRequest& req) override {
        // Layer 2: per-IP session creation rate limit
        if (rate_limiter() && !rate_limiter()->allow("session_create", req.remote_addr)) {
            return RestResponse::error(429, "Too many session creation attempts");
        }

        // max_players is a legacy UI hint only — not enforced server-side.
        // AO servers historically advertised a player limit, but modern
        // usage treats it as cosmetic. Players are silently admitted
        // beyond the advertised count.

        if (!req.body) {
            return RestResponse::error(400, "Request body is required");
        }

        auto& body = *req.body;

        if (auto err = aonx_request_schema("createSession").validate(body); !err.empty()) {
            return RestResponse::error(400, err);
        }

        auto client_name = body.value("client_name", std::string{});
        auto client_version = body.value("client_version", std::string{});
        auto hdid = body.value("hdid", std::string{});

        // Sanitize: cap lengths and strip control characters.
        auto sanitize = [](std::string& s, size_t max_len) {
            if (s.size() > max_len)
                s.resize(max_len);
            std::erase_if(s, [](unsigned char c) { return c < 0x20; });
        };
        sanitize(client_name, 64);
        sanitize(client_version, 32);
        sanitize(hdid, 128);

        auto info = server().create_session(hdid, client_name, client_version, req.remote_addr);

        auto roles = nlohmann::json::array({"player"});
        if (info.moderator)
            roles.push_back("moderator");

        nlohmann::json resp = {
            {"token", info.token},
            {"user",
             {
                 {"id", std::to_string(info.session_id)},
                 {"display_name", client_name},
                 {"roles", std::move(roles)},
             }},
        };

        int ttl = server().session_ttl_seconds();
        if (ttl > 0) {
            auto expires_at = std::chrono::system_clock::now() + std::chrono::seconds(ttl);
            resp["expires_at"] = to_iso8601(expires_at);
        }

        return RestResponse::json(201, std::move(resp));
    }
};

EndpointRegistrar reg("POST /aonx/v1/session", [] { return std::make_unique<SessionCreateEndpoint>(); });

} // namespace

// Linker anchor — referenced by nx_register_endpoints().
void nx_ep_session_create() {
}
