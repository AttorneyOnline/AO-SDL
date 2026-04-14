#pragma once

#include <functional>
#include <string>
#include <vector>

struct ServerContext;

/// Summary of a config reload operation.
struct ReloadResult {
    bool ok = true;
    std::string error;

    /// Human-readable lines describing what changed.
    std::vector<std::string> reloaded;

    /// Settings that changed but require a full restart to take effect.
    std::vector<std::string> restart_warnings;

    /// Number of players relocated from removed areas.
    int players_relocated = 0;

    /// Number of players whose character was unselected (character removed).
    int characters_unselected = 0;

    /// Whether content lists changed (characters, music, areas, backgrounds).
    bool content_changed = false;

    /// Format the result into a multi-line summary string.
    std::string format() const;
};

/// Callable that runs a function under the dispatch lock.
/// REPL callers pass: [&](auto fn) { ctx.rest_router.with_lock(fn); }
/// In-game callers (already locked) pass: [](auto fn) { fn(); }
using LockWrapper = std::function<void(std::function<void()>)>;

/// Perform a full hot-reload of server and content configuration.
///
/// Phase 1 (Read): re-reads kagami.json and content config from disk.
/// Phase 2 (Diff): snapshots restart-required keys, applies new JSON,
///                  detects restart-required changes.
/// Phase 3 (Apply): inside the dispatch lock, updates all subsystems.
ReloadResult perform_reload(ServerContext& ctx, LockWrapper lock_wrapper);
