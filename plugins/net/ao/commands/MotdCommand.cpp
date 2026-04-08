#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"

class MotdCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "motd";
        return n;
    }

    std::string usage() const override {
        return "/motd";
    }

    void execute(CommandContext& ctx) override {
        auto& desc = ctx.room.server_description;
        if (desc.empty())
            ctx.send_system_message("No message of the day set.");
        else
            ctx.send_system_message(desc);
    }
};

static CommandRegistrar reg(std::make_unique<MotdCommand>());
