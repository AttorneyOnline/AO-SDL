#include "ReplCommandRegistrar.h"
#include "ServerContext.h"
#include "ServerSettings.h"
#include "TerminalUI.h"

#include "net/WebSocketServer.h"

namespace {

class StatusCommand : public ReplCommand {
  public:
    const std::string& name() const override {
        static const std::string n = "/status";
        return n;
    }
    const std::string& description() const override {
        static const std::string d = "Show server status";
        return d;
    }
    void execute(ServerContext& ctx, const std::vector<std::string>&) override {
        ctx.ui.print("Server:     " + ctx.cfg.server_name());
        ctx.ui.print("HTTP:       " + ctx.cfg.bind_address() + ":" + std::to_string(ctx.cfg.http_port()));
        ctx.ui.print("WS:         " + ctx.cfg.bind_address() + ":" + std::to_string(ctx.cfg.ws_port()));
        if (ctx.ws)
            ctx.ui.print("WS clients: " + std::to_string(ctx.ws->client_count()));
    }
};

ReplCommandRegistrar reg("/status", [] { return std::make_unique<StatusCommand>(); });

} // namespace

void repl_cmd_status() {}
