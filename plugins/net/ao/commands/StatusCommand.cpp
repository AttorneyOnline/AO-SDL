#include "game/AreaState.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

/// /status <status> — set area status (IDLE, CASING, RECESS, etc.)
class AreaStatusCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "status";
        return n;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/status <IDLE|CASING|RECESS|LOOKING-FOR-PLAYERS|GAMING>";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;

        // Uppercase the status
        std::string status = ctx.args[1];
        for (auto& c : status)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        // Validate
        if (status != "IDLE" && status != "CASING" && status != "RECESS" && status != "LOOKING-FOR-PLAYERS" &&
            status != "GAMING") {
            ctx.send_system_message("Invalid status. Use: IDLE, CASING, RECESS, LOOKING-FOR-PLAYERS, GAMING");
            return;
        }

        area->status = status;
        ctx.send_system_message("Area status set to " + status + ".");
    }
};

static CommandRegistrar reg(std::make_unique<AreaStatusCommand>());
void ao_cmd_status() {}
