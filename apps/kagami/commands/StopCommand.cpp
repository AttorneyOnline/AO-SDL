#include "ReplCommandRegistrar.h"
#include "ServerContext.h"

namespace {

class StopCommand : public SimpleReplCommand {
  public:
    StopCommand() : SimpleReplCommand("/stop", "Shut down the server") {
    }
    void execute(ServerContext& ctx, const std::vector<std::string>&) override {
        ctx.stop_src.request_stop();
    }
};

ReplCommandRegistrar reg("/stop", [] { return std::make_unique<StopCommand>(); });

} // namespace

void repl_cmd_stop() {
}
