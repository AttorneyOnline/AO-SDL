#include "ReplCommandRegistrar.h"
#include "ServerContext.h"

namespace {

class StopCommand : public ReplCommand {
  public:
    const std::string& name() const override {
        static const std::string n = "/stop";
        return n;
    }
    const std::string& description() const override {
        static const std::string d = "Shut down the server";
        return d;
    }
    void execute(ServerContext& ctx, const std::vector<std::string>&) override {
        ctx.stop_src.request_stop();
    }
};

ReplCommandRegistrar reg("/stop", [] { return std::make_unique<StopCommand>(); });

} // namespace

void repl_cmd_stop() {}
