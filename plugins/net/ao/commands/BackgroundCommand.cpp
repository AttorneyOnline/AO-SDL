#include "game/AreaState.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "net/ao/AOPacket.h"

/// /background <name> — change the area background.
class BackgroundCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "background";
        return n;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/background <name>";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;

        if (area->bg_locked && !ctx.session.moderator && !area->is_cm(ctx.session.client_id)) {
            ctx.send_system_message("The background is locked in this area.");
            return;
        }

        std::string bg;
        for (size_t i = 1; i < ctx.args.size(); ++i) {
            if (!bg.empty())
                bg += ' ';
            bg += ctx.args[i];
        }

        area->background.name = bg;

        // Broadcast BN to all clients in the area
        if (ctx.broadcast_background)
            ctx.broadcast_background(ctx.session.area, bg);
        ctx.send_system_message("Background changed to " + bg + ".");
    }
};

/// /lock_background — lock the background in this area.
class LockBackgroundCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "lock_background";
        return n;
    }

    bool requires_moderator() const override {
        return false;
    }

    std::string usage() const override {
        return "/lock_background";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;
        if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
            ctx.send_system_message("You must be CM or moderator to lock the background.");
            return;
        }

        area->bg_locked = true;
        ctx.send_system_message("Background locked.");
    }
};

/// /unlock_background — unlock the background in this area.
class UnlockBackgroundCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "unlock_background";
        return n;
    }

    std::string usage() const override {
        return "/unlock_background";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;
        if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
            ctx.send_system_message("You must be CM or moderator to unlock the background.");
            return;
        }

        area->bg_locked = false;
        ctx.send_system_message("Background unlocked.");
    }
};

static CommandRegistrar reg1(std::make_unique<BackgroundCommand>());
static CommandRegistrar reg2(std::make_unique<LockBackgroundCommand>());
static CommandRegistrar reg3(std::make_unique<UnlockBackgroundCommand>());
void ao_cmd_background() {
}
