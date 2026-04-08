#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"

#include <algorithm>

class HelpCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "help";
        return n;
    }

    std::string usage() const override {
        return "/help";
    }

    void execute(CommandContext& ctx) override {
        auto commands = CommandRegistry::instance().all_commands();
        std::sort(commands.begin(), commands.end(),
                  [](const CommandHandler* a, const CommandHandler* b) { return a->name() < b->name(); });

        std::string text = "Available commands:";
        for (auto* cmd : commands) {
            text += "\n  " + cmd->usage();
            if (cmd->requires_moderator())
                text += " [MOD]";
        }
        ctx.send_system_message(text);
    }
};

static CommandRegistrar reg(std::make_unique<HelpCommand>());
void ao_cmd_help() {}
