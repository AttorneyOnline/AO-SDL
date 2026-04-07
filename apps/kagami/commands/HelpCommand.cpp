#include "ReplCommand.h"
#include "ReplCommandRegistrar.h"
#include "ServerContext.h"
#include "TerminalUI.h"

namespace {

class HelpCommand : public ReplCommand {
  public:
    const std::string& name() const override {
        static const std::string n = "/help";
        return n;
    }
    const std::string& description() const override {
        static const std::string d = "List available commands";
        return d;
    }
    void execute(ServerContext& ctx, const std::vector<std::string>&) override {
        if (!ctx.repl)
            return;
        for (auto& [cmd_name, cmd] : ctx.repl->commands())
            ctx.ui.print("  " + cmd_name + " — " + cmd->description());
    }
};

ReplCommandRegistrar reg("/help", [] { return std::make_unique<HelpCommand>(); });

} // namespace

void repl_cmd_help() {}
