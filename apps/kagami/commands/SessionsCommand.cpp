#include "ReplCommandRegistrar.h"
#include "ServerContext.h"
#include "TerminalUI.h"

#include "game/ClientId.h"
#include "game/GameRoom.h"
#include "net/RestRouter.h"

namespace {

class SessionsCommand : public SimpleReplCommand {
  public:
    SessionsCommand() : SimpleReplCommand("/sessions", "List active sessions") {
    }
    void execute(ServerContext& ctx, const std::vector<std::string>&) override {
        ctx.rest_router.with_lock([&] {
            if (ctx.room.session_count() == 0) {
                ctx.ui.print("  No active sessions.");
                return;
            }
            ctx.room.for_each_session([&](const ServerSession& s) {
                std::string info = "  " + format_client_id(s.client_id);
                if (!s.display_name.empty())
                    info += " \"" + s.display_name + "\"";
                if (s.character_id >= 0)
                    info += " char=" + std::to_string(s.character_id);
                if (!s.area.empty())
                    info += " area=" + s.area;
                if (!s.client_software.empty())
                    info += " (" + s.client_software + ")";
                ctx.ui.print(info);
            });
            ctx.ui.print("  Total: " + std::to_string(ctx.room.session_count()));
        });
    }
};

ReplCommandRegistrar reg("/sessions", [] { return std::make_unique<SessionsCommand>(); });

} // namespace

void repl_cmd_sessions() {
}
