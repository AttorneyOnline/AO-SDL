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

        // Compare SHA-256 hash of the attempt against the configured value.
        // The config stores the password as a SHA-256 hex hash.
        // If the configured value is 64 hex chars, it's already hashed.
        // Otherwise, treat it as plaintext (for backwards compatibility)
        // and compare directly.
        std::string attempt_hash = crypto::sha256(ctx.args[1]);
        bool match = false;
        if (configured.size() == 64) {
            // Configured value is a hash — compare hash-to-hash
            match = (attempt_hash == configured);
        }
        else {
            // Configured value is plaintext — compare directly
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
