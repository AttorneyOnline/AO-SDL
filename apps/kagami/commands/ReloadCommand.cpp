#include "ConfigReloader.h"
#include "ReplCommandRegistrar.h"
#include "ServerContext.h"
#include "TerminalUI.h"

#include "net/RestRouter.h"

#include <sstream>

namespace {

class ReloadCommand : public SimpleReplCommand {
  public:
    ReloadCommand() : SimpleReplCommand("/reload", "Hot-reload config from disk") {
    }
    void execute(ServerContext& ctx, const std::vector<std::string>&) override {
        ctx.ui.print("[reload] Reloading configuration...");

        auto result = perform_reload(ctx, [&](std::function<void()> fn) { ctx.rest_router.with_lock(fn); });

        // Print each line of the formatted result.
        auto summary = result.format();
        std::istringstream stream(summary);
        std::string line;
        while (std::getline(stream, line))
            ctx.ui.print(line);
    }
};

ReplCommandRegistrar reg("/reload", [] { return std::make_unique<ReloadCommand>(); });

} // namespace

void repl_cmd_reload() {
}
