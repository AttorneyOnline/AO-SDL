#include "game/AreaState.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

/// /getarea — show info about the current area.
class GetAreaCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "getarea";
        return n;
    }

    std::string usage() const override {
        return "/getarea";
    }

    void execute(CommandContext& ctx) override {
        auto& room = ctx.room;
        auto sessions = room.sessions_in_area(ctx.session.area);
        auto* area = room.find_area_by_name(ctx.session.area);

        std::string text = "=== " + ctx.session.area + " ===";
        if (area) {
            text += "\nStatus: " + area->status;

            const char* lock_str = "FREE";
            if (area->lock_mode == AreaLockMode::LOCKED)
                lock_str = "LOCKED";
            else if (area->lock_mode == AreaLockMode::SPECTATABLE)
                lock_str = "SPECTATABLE";
            text += std::string(" | Lock: ") + lock_str;

            if (!area->cm.empty())
                text += " | CM: " + area->cm;
            if (!area->background.name.empty())
                text += "\nBackground: " + area->background.name;
        }

        text += "\nPlayers (" + std::to_string(sessions.size()) + "):";
        for (auto* s : sessions) {
            text += "\n  ";
            text += s->display_name.empty() ? "(spectator)" : s->display_name;
            text += " [" + s->ipid + "]";
            if (s->moderator)
                text += " [M]";
        }
        ctx.send_system_message(text);
    }
};

/// /getareas — show all areas with player counts.
class GetAreasCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "getareas";
        return n;
    }

    std::string usage() const override {
        return "/getareas";
    }

    void execute(CommandContext& ctx) override {
        auto& room = ctx.room;
        std::string text = "Areas:";
        for (int i = 0; i < static_cast<int>(room.areas.size()); ++i) {
            auto sessions = room.sessions_in_area(room.areas[i]);
            auto* area = room.find_area_by_name(room.areas[i]);

            text += "\n  [" + std::to_string(i) + "] " + room.areas[i];
            text += " (" + std::to_string(sessions.size()) + " players)";
            if (area) {
                if (area->status != "IDLE")
                    text += " [" + area->status + "]";
                if (area->lock_mode == AreaLockMode::LOCKED)
                    text += " [LOCKED]";
                else if (area->lock_mode == AreaLockMode::SPECTATABLE)
                    text += " [SPECTATABLE]";
                if (!area->cm.empty())
                    text += " CM: " + area->cm;
            }
        }
        ctx.send_system_message(text);
    }
};

static CommandRegistrar reg1(std::make_unique<GetAreaCommand>());
static CommandRegistrar reg2(std::make_unique<GetAreasCommand>());
void ao_cmd_getarea() {}
