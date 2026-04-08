#include "game/AreaState.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Log.h"

/// /cm — become case master of the current area, or add another user.
class CmCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "cm";
        return n;
    }

    std::string usage() const override {
        return "/cm [ipid]";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;

        if (ctx.args.size() > 1) {
            // Adding another user as CM — requires existing CM or mod
            if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
                ctx.send_system_message("You must be CM or moderator to add other CMs.");
                return;
            }

            auto& target_ipid = ctx.args[1];
            bool found = false;
            ctx.room.for_each_session([&](ServerSession& s) {
                if (s.ipid == target_ipid && s.area == ctx.session.area) {
                    area->cm_owners.insert(s.client_id);
                    ctx.send_system_message_to(s.client_id, "You are now a case master.");
                    found = true;
                }
            });
            if (found)
                ctx.send_system_message("Added CM for IPID " + target_ipid + ".");
            else
                ctx.send_system_message("No player with IPID " + target_ipid + " in this area.");
        }
        else {
            // Self-CM
            area->cm_owners.insert(ctx.session.client_id);
            area->cm = ctx.session.display_name;
            area->invited.insert(ctx.session.client_id);
            ctx.send_system_message("You are now a case master of " + ctx.session.area + ".");
            Log::log_print(INFO, "CM: %s became CM of %s", ctx.session.display_name.c_str(), ctx.session.area.c_str());
        }
    }
};

/// /uncm — step down as CM, or remove another CM (requires CM/mod).
class UncmCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "uncm";
        return n;
    }

    std::string usage() const override {
        return "/uncm [ipid]";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;

        if (ctx.args.size() > 1) {
            // Removing another CM — requires mod
            if (!ctx.session.moderator) {
                ctx.send_system_message("You must be a moderator to remove other CMs.");
                return;
            }

            auto& target_ipid = ctx.args[1];
            bool found = false;
            ctx.room.for_each_session([&](ServerSession& s) {
                if (s.ipid == target_ipid) {
                    area->cm_owners.erase(s.client_id);
                    found = true;
                }
            });
            if (found)
                ctx.send_system_message("Removed CM for IPID " + target_ipid + ".");
            else
                ctx.send_system_message("No player with IPID " + target_ipid + " found.");
        }
        else {
            // Step down from CM
            if (!area->is_cm(ctx.session.client_id)) {
                ctx.send_system_message("You are not a CM of this area.");
                return;
            }
            area->cm_owners.erase(ctx.session.client_id);
            if (area->cm_owners.empty())
                area->cm.clear();
            ctx.send_system_message("You are no longer a case master.");
        }
    }
};

/// /invite <ipid> — invite a user to the locked area.
class InviteCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "invite";
        return n;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/invite <ipid>";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;
        if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
            ctx.send_system_message("You must be CM or moderator to invite.");
            return;
        }

        auto& target_ipid = ctx.args[1];
        bool found = false;
        ctx.room.for_each_session([&](ServerSession& s) {
            if (s.ipid == target_ipid) {
                area->invited.insert(s.client_id);
                ctx.send_system_message_to(s.client_id, "You have been invited to " + ctx.session.area + ".");
                found = true;
            }
        });

        if (found)
            ctx.send_system_message("Invited IPID " + target_ipid + ".");
        else
            ctx.send_system_message("No player with IPID " + target_ipid + " found.");
    }
};

/// /uninvite <ipid> — revoke an invite.
class UninviteCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "uninvite";
        return n;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/uninvite <ipid>";
    }

    void execute(CommandContext& ctx) override {
        auto* area = ctx.room.find_area_by_name(ctx.session.area);
        if (!area)
            return;
        if (!area->is_cm(ctx.session.client_id) && !ctx.session.moderator) {
            ctx.send_system_message("You must be CM or moderator to uninvite.");
            return;
        }

        auto& target_ipid = ctx.args[1];
        bool found = false;
        ctx.room.for_each_session([&](ServerSession& s) {
            if (s.ipid == target_ipid) {
                area->invited.erase(s.client_id);
                found = true;
            }
        });

        if (found)
            ctx.send_system_message("Uninvited IPID " + target_ipid + ".");
        else
            ctx.send_system_message("No player with IPID " + target_ipid + " found.");
    }
};

static CommandRegistrar reg1(std::make_unique<CmCommand>());
static CommandRegistrar reg2(std::make_unique<UncmCommand>());
static CommandRegistrar reg3(std::make_unique<InviteCommand>());
static CommandRegistrar reg4(std::make_unique<UninviteCommand>());
