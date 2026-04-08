#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/DatabaseManager.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Crypto.h"
#include "utils/Log.h"

#include <chrono>

/// Cooldown between login attempts (seconds). Doubles with each failure, capped at 60s.
static int login_cooldown_seconds(int failures) {
    if (failures <= 0)
        return 0;
    int secs = 1 << failures; // 2^failures
    return secs > 60 ? 60 : secs;
}

class LoginCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "login";
        return n;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/login <password>  OR  /login <username> <password>";
    }

    void execute(CommandContext& ctx) override {
        if (ctx.session.moderator) {
            ctx.send_system_message("You are already logged in.");
            return;
        }

        // Rate-limit login attempts
        int cooldown = login_cooldown_seconds(ctx.session.login_failures);
        if (cooldown > 0) {
            auto elapsed = std::chrono::steady_clock::now() - ctx.session.last_login_failure;
            if (elapsed < std::chrono::seconds(cooldown)) {
                auto remaining =
                    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::seconds(cooldown) - elapsed).count();
                ctx.send_system_message("Too many failed attempts. Try again in " + std::to_string(remaining) + "s.");
                return;
            }
        }

        if (ctx.room.auth_type == AuthType::ADVANCED) {
            login_advanced(ctx);
        }
        else {
            login_simple(ctx);
        }
    }

  private:
    void login_simple(CommandContext& ctx) {
        auto& configured = ctx.room.mod_password;
        if (configured.empty()) {
            ctx.send_system_message("Authentication is not configured on this server.");
            return;
        }

        // Compare the attempt against the configured password.
        // Config format: "sha256:abcdef..." or plaintext
        bool match = false;
        static const std::string hash_prefix = "sha256:";
        if (configured.size() > hash_prefix.size() && configured.compare(0, hash_prefix.size(), hash_prefix) == 0) {
            std::string stored_hash = configured.substr(hash_prefix.size());
            match = (crypto::sha256(ctx.args[1]) == stored_hash);
        }
        else {
            match = (ctx.args[1] == configured);
        }

        if (!match) {
            login_failed(ctx);
            return;
        }

        // SIMPLE mode: authenticated users get SUPER permissions.
        ctx.session.moderator = true;
        ctx.session.acl_role = "SUPER";
        ctx.session.moderator_name.clear();
        ctx.session.login_failures = 0;
        ctx.room.stats.moderators.fetch_add(1, std::memory_order_relaxed);

        Log::log_print(INFO, "Auth: %s [%s] logged in (simple)", ctx.session.display_name.c_str(),
                       ctx.session.ipid.c_str());
        ctx.send_system_message("Logged in as moderator.");
    }

    void login_advanced(CommandContext& ctx) {
        // ADVANCED mode: /login <username> <password>
        int arg_count = static_cast<int>(ctx.args.size()) - 1;
        if (arg_count < 2) {
            ctx.send_system_message("Usage: /login <username> <password>");
            return;
        }

        auto* db = ctx.room.db_manager();
        if (!db || !db->is_open()) {
            ctx.send_system_message("Database is not available.");
            return;
        }

        auto& username = ctx.args[1];
        auto& password = ctx.args[2];

        // Look up user in database (blocks on future)
        auto user_opt = db->get_user(username).get();
        if (!user_opt) {
            login_failed(ctx);
            return;
        }

        auto& user = *user_opt;

        // Hash the provided password with the stored salt and compare.
        // Compatible with akashi: salt >= 16 bytes (32 hex chars) → PBKDF2.
        std::string computed_hash;
        if (user.salt.size() >= 32) {
            // PBKDF2-SHA256 with 100,000 iterations (akashi default)
            computed_hash = crypto::pbkdf2_sha256_hex(password, user.salt);
        }
        else {
            // Legacy: HMAC-SHA256 (salt < 16 bytes)
            computed_hash = crypto::hmac_sha256_hex(user.salt, password);
        }

        if (computed_hash != user.password) {
            login_failed(ctx);
            return;
        }

        ctx.session.moderator = true;
        ctx.session.acl_role = user.acl;
        ctx.session.moderator_name = user.username;
        ctx.session.login_failures = 0;
        ctx.room.stats.moderators.fetch_add(1, std::memory_order_relaxed);

        auto perms = acl_permissions_for_role(user.acl);
        Log::log_print(INFO, "Auth: %s [%s] logged in as %s (role: %s)", ctx.session.display_name.c_str(),
                       ctx.session.ipid.c_str(), user.username.c_str(), user.acl.c_str());
        ctx.send_system_message("Logged in as " + user.username + " [" + user.acl + "].");
    }

    void login_failed(CommandContext& ctx) {
        ++ctx.session.login_failures;
        ctx.session.last_login_failure = std::chrono::steady_clock::now();
        Log::log_print(WARNING, "Auth: failed login attempt from %s [%s] (attempt %d)",
                       ctx.session.display_name.c_str(), ctx.session.ipid.c_str(), ctx.session.login_failures);
        ctx.send_system_message("Invalid credentials.");
    }
};

static CommandRegistrar reg(std::make_unique<LoginCommand>());
void ao_cmd_login() {}
