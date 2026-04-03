#include "net/nx/NXEndpoint.h"

#include "net/EndpointRegistrar.h"
#include "utils/GeneratedSchemas.h"

#include <chrono>
#include <format>

namespace {

/// Format a system_clock time_point as ISO 8601 (UTC).
std::string to_iso8601(std::chrono::system_clock::time_point tp) {
    auto dp = std::chrono::floor<std::chrono::days>(tp);
    std::chrono::year_month_day ymd{dp};
    std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::seconds>(tp - dp)};
    return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z", static_cast<int>(ymd.year()),
                       static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()), hms.hours().count(),
                       hms.minutes().count(), hms.seconds().count());
}

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

    RestResponse handle(const RestRequest& req) override {
        if (room().session_count() >= static_cast<size_t>(room().max_players)) {
            return RestResponse::error(503, "Server is full");
        }

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

        auto token = server().create_session(hdid, client_name, client_version);

        nlohmann::json resp = {
            {"token", token},
            {"user",
             {
                 {"id", ""},
                 {"display_name", client_name},
                 {"roles", nlohmann::json::array({"player"})},
             }},
        };

        // Fill in the server-assigned user id.
        if (auto* session = room().find_session_by_token(token)) {
            resp["user"]["id"] = std::to_string(session->session_id);
            if (session->moderator)
                resp["user"]["roles"].push_back("moderator");
        }

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
