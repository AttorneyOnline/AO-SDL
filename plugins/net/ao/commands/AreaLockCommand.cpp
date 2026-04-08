#include "game/AreaState.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

/// /area_lock — lock the area (invite-only).
class AreaLockCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "area_lock";
        return n;
    }

    std::string usage() const override {
        return "/area_lock";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;
        if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
            ctx.send_system_message("You must be CM or moderator to lock the area.");
            return;
        }

        area->lock_mode = AreaLockMode::LOCKED;
        // Auto-invite all current players
        for (auto* s : ctx.room.sessions_in_area(ctx.session.area))
            area->invited.insert(s->client_id);

        ctx.send_system_message("Area locked. Current players have been invited.");
    }
};

/// /area_unlock — unlock the area (free for all).
class AreaUnlockCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "area_unlock";
        return n;
    }

    std::string usage() const override {
        return "/area_unlock";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;
        if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
            ctx.send_system_message("You must be CM or moderator to unlock the area.");
            return;
        }

        area->lock_mode = AreaLockMode::FREE;
        area->invited.clear();
        ctx.send_system_message("Area unlocked.");
    }
};

/// /area_spectate — set area to spectatable mode.
class AreaSpectateCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "area_spectate";
        return n;
    }

    std::string usage() const override {
        return "/area_spectate";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;
        if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
            ctx.send_system_message("You must be CM or moderator to set area to spectatable.");
            return;
        }

        area->lock_mode = AreaLockMode::SPECTATABLE;
        // Auto-invite all current players
        for (auto* s : ctx.room.sessions_in_area(ctx.session.area))
            area->invited.insert(s->client_id);

        ctx.send_system_message("Area set to spectatable. Current players can speak, newcomers can only watch.");
    }
};

/// /area_kick <ipid> — kick user from area back to lobby.
class AreaKickCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "area_kick";
        return n;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/area_kick <ipid>";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;
        if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
            ctx.send_system_message("You must be CM or moderator to area-kick.");
            return;
        }

        auto& target_ipid = ctx.args[1];
        auto& room = ctx.room;
        std::string lobby = room.areas.empty() ? "" : room.areas[0];
        int kicked = 0;

        room.for_each_session([&](ServerSession& s) {
            if (s.ipid == target_ipid && s.area == ctx.session.area) {
                s.area = lobby;
                area->invited.erase(s.client_id);
                ctx.send_system_message_to(s.client_id, "You have been kicked from the area.");
                ++kicked;
            }
        });

        if (kicked > 0)
            ctx.send_system_message("Kicked " + std::to_string(kicked) + " player(s) from the area.");
        else
            ctx.send_system_message("No players with IPID " + target_ipid + " found in this area.");
    }
};

static CommandRegistrar reg1(std::make_unique<AreaLockCommand>());
static CommandRegistrar reg2(std::make_unique<AreaUnlockCommand>());
static CommandRegistrar reg3(std::make_unique<AreaSpectateCommand>());
static CommandRegistrar reg4(std::make_unique<AreaKickCommand>());
