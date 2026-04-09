#include "game/ACLFlags.h"
#include "game/BanManager.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"

#include "utils/Log.h"

class EditBanCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "editban";
        return n;
    }

    ACLPermission required_permission() const override {
        return ACLPermission::BAN;
    }

    int min_args() const override {
        return 3;
    }

    std::string usage() const override {
        return "/editban <ipid> <reason|duration> <value>";
    }

    void execute(CommandContext& ctx) override {
        auto* bm = ctx.room.ban_manager();
        if (!bm) {
            ctx.send_system_message("Ban system is not configured.");
            return;
        }

        auto& target_ipid = ctx.args[1];
        auto& field = ctx.args[2];

        if (field != "reason" && field != "duration") {
            ctx.send_system_message("Field must be 'reason' or 'duration'. Usage: " + usage());
            return;
        }

        // Build value from remaining args (allows spaces in reason)
        std::string value;
        for (size_t i = 3; i < ctx.args.size(); ++i) {
            if (!value.empty())
                value += ' ';
            value += ctx.args[i];
        }

        if (field == "duration") {
            int64_t dur = parse_ban_duration(value);
            if (dur == -1) {
                ctx.send_system_message("Invalid duration: " + value + ". Use 'perma', '30m', '1h', etc.");
                return;
            }
        }

        if (bm->update_ban(target_ipid, field, value)) {
            Log::log_print(INFO, "EditBan: %s [%s] updated ban %s: %s = %s", ctx.session.display_name.c_str(),
                           ctx.session.ipid.c_str(), target_ipid.c_str(), field.c_str(), value.c_str());
            ctx.send_system_message("Updated ban " + target_ipid + ": " + field + " = " + value);
        }
        else {
            ctx.send_system_message("No active ban found for IPID " + target_ipid + ", or invalid field.");
        }
    }
};

static CommandRegistrar reg(std::make_unique<EditBanCommand>());
void ao_cmd_editban() {
}
