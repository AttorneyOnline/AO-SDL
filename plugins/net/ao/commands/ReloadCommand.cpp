#include "game/ACLFlags.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"

#include <sstream>

/// /reload -- Hot-reload server and content configuration from disk.
/// Requires SUPER permission (server-wide impact).
class ReloadCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "reload";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::SUPER;
    }

    int min_args() const override {
        return 0;
    }

    std::string usage() const override {
        return "/reload";
    }

    void execute(CommandContext& ctx) override {
        const auto& reload = ctx.room.reload_func();
        if (!reload) {
            ctx.send_system_message("Reload not available (not wired by server).");
            return;
        }

        ctx.send_system_message("Reloading configuration...");
        auto summary = reload();

        // Send each line as a separate system message so the client
        // doesn't truncate long output.
        std::istringstream stream(summary);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty())
                ctx.send_system_message(line);
        }
    }
};

static CommandRegistrar reg(std::make_unique<ReloadCommand>());
void ao_cmd_reload() {
}
