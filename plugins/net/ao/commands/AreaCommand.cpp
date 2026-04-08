#include "game/AreaState.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Log.h"

/// /area <name> — switch to a different area.
class AreaCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "area";
        return n;
    }

    int min_args() const override {
        return 1;
    }

    std::string usage() const override {
        return "/area <name>";
    }

    void execute(CommandContext& ctx) override {
        // Rejoin remaining args as area name (areas can have spaces)
        std::string target;
        for (size_t i = 1; i < ctx.args.size(); ++i) {
            if (!target.empty())
                target += ' ';
            target += ctx.args[i];
        }

        auto& room = ctx.room;

        // Find the area by name (case-insensitive prefix match if exact fails)
        std::string matched;
        for (auto& area_name : room.areas) {
            if (area_name == target) {
                matched = area_name;
                break;
            }
        }
        // Try case-insensitive match
        if (matched.empty()) {
            std::string lower_target = target;
            for (auto& c : lower_target)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            for (auto& area_name : room.areas) {
                std::string lower_name = area_name;
                for (auto& c : lower_name)
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (lower_name == lower_target) {
                    matched = area_name;
                    break;
                }
            }
        }
        // Try by index
        if (matched.empty()) {
            try {
                int idx = std::stoi(target);
                if (idx >= 0 && idx < static_cast<int>(room.areas.size()))
                    matched = room.areas[idx];
            }
            catch (...) {
            }
        }

        if (matched.empty()) {
            ctx.send_system_message("Area not found: " + target);
            return;
        }

        // Check if area allows entry
        auto* area = room.find_area_by_name(matched);
        if (area && !area->can_enter(ctx.session.client_id)) {
            ctx.send_system_message("Area " + matched + " is locked. You need an invite.");
            return;
        }

        std::string old_area = ctx.session.area;
        ctx.session.area = matched;

        // Send area-join info (BN, HP, LE) to the client
        if (ctx.send_area_join_info)
            ctx.send_area_join_info(ctx.session.client_id, matched);

        Log::log_print(INFO, "Command: %s moved from %s to %s", ctx.session.display_name.c_str(), old_area.c_str(),
                       matched.c_str());
        ctx.send_system_message("Moved to " + matched + ".");
    }
};

static CommandRegistrar reg(std::make_unique<AreaCommand>());
