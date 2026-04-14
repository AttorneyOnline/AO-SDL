#include "ReplCommandFactory.h"

// Linker anchors — calling these forces the linker to include each
// command's translation unit, which triggers its static ReplCommandRegistrar.
void repl_cmd_stop();
void repl_cmd_help();
void repl_cmd_sessions();
void repl_cmd_status();
void repl_cmd_reload();

void kagami_register_commands() {
    repl_cmd_stop();
    repl_cmd_help();
    repl_cmd_sessions();
    repl_cmd_status();
    repl_cmd_reload();
}

void ReplCommandFactory::populate(ReplCommandRegistry& registry) {
    for (auto& [name, creator] : creators_)
        registry.add(creator());
}
