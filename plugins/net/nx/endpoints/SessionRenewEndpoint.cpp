#include "net/EndpointRegistrar.h"
#include "net/nx/NXEndpoint.h"

#include <chrono>
#include <format>

namespace {

/// Format a system_clock time_point as ISO 8601 (UTC).
/// Uses C++20 chrono calendar types — no gmtime_r/gmtime_s portability issues.
std::string to_iso8601(std::chrono::system_clock::time_point tp) {
    auto dp = std::chrono::floor<std::chrono::days>(tp);
    std::chrono::year_month_day ymd{dp};
    std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::seconds>(tp - dp)};
    return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z", static_cast<int>(ymd.year()),
                       static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()), hms.hours().count(),
                       hms.minutes().count(), hms.seconds().count());
}

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
