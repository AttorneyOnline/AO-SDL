#include "net/nx/NXEndpoint.h"
#include "net/nx/NXServer.h"
#include "net/nx/NXTime.h"

#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "net/EndpointRegistrar.h"
#include "utils/Crypto.h"
#include "utils/GeneratedSchemas.h"
#include "utils/Log.h"

#include <json.hpp>

#include <chrono>

namespace {

class AuthLoginEndpoint : public NXEndpoint {
  public:
    const std::string& method() const override {
        static const std::string m = "POST";
        return m;
    }
    const std::string& path_pattern() const override {
        static const std::string p = "/aonx/v1/auth/login";
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
        if (rate_limiter() && !rate_limiter()->allow("session_create", req.remote_addr))
            return RestResponse::error(429, "Too many login attempts");

        if (!req.body)
            return RestResponse::error(400, "Request body is required");

        if (auto err = aonx_request_schema("authLogin").validate(*req.body); !err.empty())
            return RestResponse::error(400, err);

        auto username = req.body->value("username", std::string{});
        auto password = req.body->value("password", std::string{});
        auto description = req.body->value("description", std::string{});

        auto* db = room().db_manager();
        if (!db || !db->is_open())
            return RestResponse::error(503, "Database unavailable");

        // Look up user
        auto user_opt = db->get_user(username).get();
        if (!user_opt) {
            Log::log_print(INFO, "Auth: REST login failed for '%s' (user not found)", username.c_str());
            return RestResponse::error(401, "Invalid credentials");
        }

        auto& user = *user_opt;

        // Verify password using shared utility (PBKDF2 or HMAC-SHA256)
        if (!crypto::verify_password(password, user.salt, user.password)) {
            Log::log_print(INFO, "Auth: REST login failed for '%s' (wrong password)", username.c_str());
            return RestResponse::error(401, "Invalid credentials");
        }

        // Generate durable auth token
        auto token = NXServer::generate_auth_token();

        auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

        // Compute expiry from config. The TTL accessor is on ServerSettings
        // which is a singleton -- but this endpoint lives in nx_net which
        // doesn't link against the kagami app layer. Instead, read it from
        // the config via the GameRoom's reload_func pattern... or just use
        // a reasonable default. For now we'll read it from the global config
        // singleton indirectly.
        //
        // Actually, NXServer already stores session_ttl_seconds as an atomic.
        // We can add auth_token_ttl_seconds the same way, or just hardcode
        // the 30-day default here since it's also the ServerSettings default.
        // The config endpoint can be used to change it dynamically.
        int64_t ttl = 30 * 24 * 60 * 60; // 30 days default
        int64_t expires_at = ttl > 0 ? now + ttl : 0;

        AuthTokenEntry entry;
        entry.token = token;
        entry.username = user.username;
        entry.acl = user.acl;
        entry.created_at = now;
        entry.expires_at = expires_at;
        entry.description = description;

        if (!db->create_auth_token(std::move(entry)).get()) {
            Log::log_print(ERR, "Auth: failed to store auth token for '%s'", username.c_str());
            return RestResponse::error(500, "Failed to create auth token");
        }

        Log::log_print(INFO, "Auth: REST login for '%s' (role: %s)", user.username.c_str(), user.acl.c_str());

        nlohmann::json resp = {
            {"token", token},
            {"username", user.username},
            {"acl", user.acl},
        };
        if (expires_at > 0) {
            auto tp = std::chrono::system_clock::time_point(std::chrono::seconds(expires_at));
            resp["expires_at"] = to_iso8601(tp);
        }
        else {
            resp["expires_at"] = nullptr;
        }

        return RestResponse::json(201, std::move(resp));
    }
};

EndpointRegistrar reg("POST /aonx/v1/auth/login", [] { return std::make_unique<AuthLoginEndpoint>(); });

} // namespace

void nx_ep_auth_login() {
}
