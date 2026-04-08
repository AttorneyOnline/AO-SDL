#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Crypto.h"
#include "utils/Log.h"

#include <chrono>

/// Cooldown between login attempts (seconds). Doubles with each failure, capped at 60s.
static int login_cooldown_seconds(int failures) {
    // 0 failures = 0s, 1 = 2s, 2 = 4s, 3 = 8s, ... capped at 60s
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
        return "/login <password>";
    }

    void execute(CommandContext& ctx) override {
        if (ctx.session.moderator) {
            ctx.send_system_message("You are already logged in.");
            return;
        }

        auto& configured = ctx.room.mod_password;
        if (configured.empty()) {
            ctx.send_system_message("Authentication is not configured on this server.");
            return;
        }

        // Rate-limit login attempts
        int cooldown = login_cooldown_seconds(ctx.session.login_failures);
        if (cooldown > 0) {
            auto elapsed = std::chrono::steady_clock::now() - ctx.session.last_login_failure;
            if (elapsed < std::chrono::seconds(cooldown)) {
                auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
                                     std::chrono::seconds(cooldown) - elapsed)
                                     .count();
                ctx.send_system_message("Too many failed attempts. Try again in " + std::to_string(remaining) + "s.");
                return;
            }
        }

        // Compare the attempt against the configured password.
        // Config format:
        //   "sha256:abcdef..."  → stored as SHA-256 hex hash
        //   "mypassword"        → stored as plaintext (legacy)
        bool match = false;
        static const std::string hash_prefix = "sha256:";
        if (configured.size() > hash_prefix.size() &&
            configured.compare(0, hash_prefix.size(), hash_prefix) == 0) {
            // Config has a hash — compare hash-to-hash
            std::string stored_hash = configured.substr(hash_prefix.size());
            match = (crypto::sha256(ctx.args[1]) == stored_hash);
        }
        else {
            // Config has plaintext — compare directly
            match = (ctx.args[1] == configured);
        }

        if (!match) {
            ++ctx.session.login_failures;
            ctx.session.last_login_failure = std::chrono::steady_clock::now();
            Log::log_print(WARNING, "Auth: failed login attempt from %s [%s] (attempt %d)",
                           ctx.session.display_name.c_str(), ctx.session.ipid.c_str(), ctx.session.login_failures);
            ctx.send_system_message("Invalid password.");
            return;
        }

        ctx.session.moderator = true;
        ctx.session.login_failures = 0;
        ctx.room.stats.moderators.fetch_add(1, std::memory_order_relaxed);

        Log::log_print(INFO, "Auth: %s [%s] logged in as moderator", ctx.session.display_name.c_str(),
                       ctx.session.ipid.c_str());
        ctx.send_system_message("Logged in as moderator.");
    }
};

static CommandRegistrar reg(std::make_unique<LoginCommand>());
