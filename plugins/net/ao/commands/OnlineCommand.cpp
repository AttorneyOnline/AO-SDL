#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

class OnlineCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "online";
        return n;
    }

    std::string usage() const override {
        return "/online";
    }

    void execute(CommandContext& ctx) override {
        auto sessions = ctx.room.sessions_in_area(ctx.session.area);

        std::string text = "Players in " + ctx.session.area + " (" + std::to_string(sessions.size()) + "):";
        for (auto* s : sessions) {
            text += "\n  ";
            if (!s->display_name.empty())
                text += s->display_name;
            else
                text += "(spectator)";
            if (s->moderator)
                text += " [M]";
        }
        ctx.send_system_message(text);
    }
};

static CommandRegistrar reg(std::make_unique<OnlineCommand>());
