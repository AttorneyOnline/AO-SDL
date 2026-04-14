#include "game/DatabaseManager.h"

#include "utils/Log.h"

#include <sqlite3.h>

#include <algorithm>
#include <chrono>

// -- PreparedStatement --------------------------------------------------------

PreparedStatement::~PreparedStatement() {
    if (stmt_)
        sqlite3_finalize(stmt_);
}

PreparedStatement& PreparedStatement::operator=(PreparedStatement&& o) noexcept {
    if (this != &o) {
        if (stmt_)
            sqlite3_finalize(stmt_);
        stmt_ = o.stmt_;
        o.stmt_ = nullptr;
    }
    return *this;
}

// -- Worker thread ------------------------------------------------------------

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager() {
    close();
}

void DatabaseManager::worker_loop(std::stop_token stop) {
    while (!stop.stop_requested()) {
        std::function<void()> task;
        {
            std::unique_lock lock(queue_mutex_);
            queue_cv_.wait(lock, stop, [&] { return !queue_.empty(); });

            if (stop.stop_requested() && queue_.empty())
                break;
            if (queue_.empty())
                continue;

            task = std::move(queue_.front());
            queue_.pop_front();
        }
        task();
    }

    // Drain remaining tasks on shutdown so futures don't hang.
    std::deque<std::function<void()>> remaining;
    {
        std::lock_guard lock(queue_mutex_);
        remaining.swap(queue_);
    }
    for (auto& task : remaining)
        task();
}

void DatabaseManager::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(queue_mutex_);
        queue_.push_back(std::move(task));
    }
    queue_cv_.notify_one();
}

// -- Open / Close -------------------------------------------------------------

bool DatabaseManager::open(const std::string& path) {
    if (open_.load())
        close();

    // Start the worker thread, then dispatch the actual open onto it
    // so the sqlite3* handle is created on the worker thread.
    worker_ = std::jthread([this](std::stop_token st) { worker_loop(st); });

    auto future = dispatch([this, path]() -> bool {
        int rc = sqlite3_open(path.c_str(), &db_);
        if (rc != SQLITE_OK) {
            Log::log_print(ERR, "DatabaseManager: failed to open %s: %s", path.c_str(),
                           db_ ? sqlite3_errmsg(db_) : "out of memory");
            if (db_) {
                sqlite3_close(db_);
                db_ = nullptr;
            }
            return false;
        }

        // WAL mode for better concurrent read performance.
        exec("PRAGMA journal_mode=WAL");
        exec("PRAGMA foreign_keys=ON");

        if (!create_tables()) {
            Log::log_print(ERR, "DatabaseManager: failed to create tables");
            sqlite3_close(db_);
            db_ = nullptr;
            return false;
        }

        if (!migrate()) {
            Log::log_print(ERR, "DatabaseManager: migration failed");
            sqlite3_close(db_);
            db_ = nullptr;
            return false;
        }

        Log::log_print(INFO, "DatabaseManager: opened %s (SQLite %s)", path.c_str(), sqlite3_libversion());
        return true;
    });

    bool ok = future.get(); // block until init completes
    open_.store(ok);
    return ok;
}

void DatabaseManager::close() {
    if (!open_.load() && !worker_.joinable())
        return;

    open_.store(false);

    // Request stop and join — worker_loop drains the queue before exiting.
    if (worker_.joinable()) {
        worker_.request_stop();
        queue_cv_.notify_one();
        worker_.join();
    }

    // Close the handle (worker thread is gone, safe to touch db_ here).
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DatabaseManager::is_open() const {
    return open_.load();
}

// -- Schema (worker thread only) ----------------------------------------------

bool DatabaseManager::create_tables() {
    bool ok = exec("CREATE TABLE IF NOT EXISTS bans ("
                   "  ID       INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "  IPID     TEXT,"
                   "  HDID     TEXT,"
                   "  IP       TEXT,"
                   "  TIME     INTEGER,"
                   "  REASON   TEXT,"
                   "  DURATION INTEGER,"
                   "  MODERATOR TEXT"
                   ")");
    if (!ok)
        return false;

    ok = exec("CREATE TABLE IF NOT EXISTS users ("
              "  ID       INTEGER PRIMARY KEY AUTOINCREMENT,"
              "  USERNAME TEXT UNIQUE,"
              "  SALT     TEXT,"
              "  PASSWORD TEXT,"
              "  ACL      TEXT"
              ")");
    if (!ok)
        return false;

    // Content moderation audit trail. Every decision the moderator
    // makes (including LOG-only events when enabled) lands here. The
    // table is wide because we store denormalized per-axis scores so
    // operators can ad-hoc-query without joining anything.
    ok = exec("CREATE TABLE IF NOT EXISTS moderation_events ("
              "  ID              INTEGER PRIMARY KEY AUTOINCREMENT,"
              "  TIMESTAMP_MS    INTEGER NOT NULL,"
              "  IPID            TEXT,"
              "  CHANNEL         TEXT,"
              "  MESSAGE_SAMPLE  TEXT,"
              "  ACTION          TEXT,"
              "  HEAT_AFTER      REAL,"
              "  REASON          TEXT,"
              "  VISUAL_NOISE    REAL,"
              "  LINK_RISK       REAL,"
              "  HATE            REAL,"
              "  SEXUAL          REAL,"
              "  SEXUAL_MINORS   REAL,"
              "  VIOLENCE        REAL,"
              "  SELF_HARM       REAL,"
              "  SEMANTIC_ECHO   REAL"
              ")");
    if (!ok)
        return false;

    // The two indexes that matter: lookup by IPID (to answer "show me
    // all recent moderation events for this user") and by timestamp
    // (to answer "show me the last N events" cheaply). Index creation
    // checks the result for parity with the table creates above —
    // index creates almost never fail right after a successful CREATE
    // TABLE, but the inconsistency between checked-table-creates and
    // unchecked-index-creates was a code-review snag worth resolving.
    ok = exec("CREATE INDEX IF NOT EXISTS idx_mod_events_ipid ON moderation_events(IPID, TIMESTAMP_MS DESC)") &&
         exec("CREATE INDEX IF NOT EXISTS idx_mod_events_ts ON moderation_events(TIMESTAMP_MS DESC)") &&
         exec("CREATE INDEX IF NOT EXISTS idx_mod_events_action ON moderation_events(ACTION, TIMESTAMP_MS DESC)");
    if (!ok)
        return false;

    ok = exec("CREATE TABLE IF NOT EXISTS mutes ("
              "  ID          INTEGER PRIMARY KEY AUTOINCREMENT,"
              "  IPID        TEXT NOT NULL,"
              "  STARTED_AT  INTEGER NOT NULL,"
              "  EXPIRES_AT  INTEGER NOT NULL,"
              "  REASON      TEXT,"
              "  MODERATOR   TEXT"
              ")");
    if (!ok)
        return false;
    ok = exec("CREATE INDEX IF NOT EXISTS idx_mutes_ipid ON mutes(IPID)") &&
         exec("CREATE INDEX IF NOT EXISTS idx_mutes_expires ON mutes(EXPIRES_AT)");
    if (!ok)
        return false;

    ok = exec("CREATE TABLE IF NOT EXISTS auth_tokens ("
              "  ID          INTEGER PRIMARY KEY AUTOINCREMENT,"
              "  TOKEN       TEXT UNIQUE NOT NULL,"
              "  USERNAME    TEXT NOT NULL,"
              "  ACL         TEXT NOT NULL,"
              "  CREATED_AT  INTEGER NOT NULL,"
              "  EXPIRES_AT  INTEGER NOT NULL,"
              "  REVOKED     INTEGER DEFAULT 0,"
              "  LAST_USED   INTEGER DEFAULT 0,"
              "  DESCRIPTION TEXT DEFAULT ''"
              ")");
    if (!ok)
        return false;
    ok = exec("CREATE INDEX IF NOT EXISTS idx_auth_tokens_username ON auth_tokens(USERNAME)") &&
         exec("CREATE INDEX IF NOT EXISTS idx_auth_tokens_token ON auth_tokens(TOKEN)");
    return ok;
}

bool DatabaseManager::migrate() {
    auto stmt = prepare("PRAGMA user_version");
    if (!stmt)
        return false;

    int version = 0;
    if (sqlite3_step(stmt.get()) == SQLITE_ROW)
        version = sqlite3_column_int(stmt.get(), 0);

    if (version >= DB_VERSION)
        return true;

    Log::log_print(INFO, "DatabaseManager: migrating from schema v%d to v%d", version, DB_VERSION);

    // Wrap the migration in BEGIN IMMEDIATE / COMMIT so partial
    // state never lands on disk if a step fails halfway through.
    // Each `case` below advances user_version as a transactional
    // step; if any step fails the outer ROLLBACK undoes everything.
    // The DDL in create_tables() is all `CREATE IF NOT EXISTS` so
    // it's idempotent on a reopen after rollback.
    if (!exec("BEGIN IMMEDIATE"))
        return false;
    // Roll back the open transaction and report failure to the
    // caller. Only ever called on the failure path, so the previous
    // bool-conditional version of this lambda was dead code.
    auto rollback_and_fail = [this]() {
        exec("ROLLBACK");
        return false;
    };

    switch (version) {
    case 0:
        if (!exec("PRAGMA user_version = 1"))
            return rollback_and_fail();
        [[fallthrough]];
    case 1:
        // v1 → v2: moderation_events and mutes. create_tables() runs
        // CREATE IF NOT EXISTS unconditionally every open(), so there
        // is nothing to do here beyond bumping user_version.
        if (!exec("PRAGMA user_version = 2"))
            return rollback_and_fail();
        [[fallthrough]];
    case 2:
        // v2 → v3: auth_tokens for REST authentication.
        // Table is created by create_tables() unconditionally.
        if (!exec("PRAGMA user_version = 3"))
            return rollback_and_fail();
        [[fallthrough]];
    default:
        break;
    }

    if (!exec("COMMIT"))
        return rollback_and_fail();
    return true;
}

// -- Ban operations -----------------------------------------------------------

std::future<int64_t> DatabaseManager::add_ban(BanEntry entry) {
    return dispatch([this, entry = std::move(entry)]() -> int64_t {
        if (!db_)
            return -1;

        auto now = std::chrono::system_clock::now();
        int64_t time = entry.timestamp != 0
                           ? entry.timestamp
                           : std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        auto stmt = prepare("INSERT INTO bans (IPID, HDID, IP, TIME, REASON, DURATION, MODERATOR) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?)");
        if (!stmt)
            return -1;

        sqlite3_bind_text(stmt.get(), 1, entry.ipid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, entry.hdid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 3, entry.ip.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt.get(), 4, time);
        sqlite3_bind_text(stmt.get(), 5, entry.reason.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt.get(), 6, entry.duration);
        sqlite3_bind_text(stmt.get(), 7, entry.moderator.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            Log::log_print(WARNING, "DatabaseManager: add_ban failed: %s", sqlite3_errmsg(db_));
            return -1;
        }

        return sqlite3_last_insert_rowid(db_);
    });
}

std::optional<BanEntry> DatabaseManager::do_find_ban_by_ipid(const std::string& ipid) {
    if (!db_)
        return std::nullopt;

    auto stmt = prepare("SELECT * FROM bans WHERE IPID = ? ORDER BY TIME DESC");
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_text(stmt.get(), 1, ipid.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        auto entry = row_to_ban(stmt.get());
        if (is_ban_active(entry))
            return entry;
    }

    return std::nullopt;
}

std::optional<BanEntry> DatabaseManager::do_find_ban_by_hdid(const std::string& hdid) {
    if (hdid.empty() || !db_)
        return std::nullopt;

    auto stmt = prepare("SELECT * FROM bans WHERE HDID = ? ORDER BY TIME DESC");
    if (!stmt)
        return std::nullopt;

    sqlite3_bind_text(stmt.get(), 1, hdid.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        auto entry = row_to_ban(stmt.get());
        if (is_ban_active(entry))
            return entry;
    }

    return std::nullopt;
}

std::future<std::optional<BanEntry>> DatabaseManager::find_ban_by_ipid(std::string ipid) {
    return dispatch([this, ipid = std::move(ipid)]() { return do_find_ban_by_ipid(ipid); });
}

std::future<std::optional<BanEntry>> DatabaseManager::find_ban_by_hdid(std::string hdid) {
    return dispatch([this, hdid = std::move(hdid)]() { return do_find_ban_by_hdid(hdid); });
}

std::future<std::optional<BanEntry>> DatabaseManager::check_ban(std::string ipid, std::string hdid) {
    return dispatch([this, ipid = std::move(ipid), hdid = std::move(hdid)]() -> std::optional<BanEntry> {
        auto result = do_find_ban_by_ipid(ipid);
        if (result)
            return result;
        return do_find_ban_by_hdid(hdid);
    });
}

std::future<bool> DatabaseManager::invalidate_ban(int64_t ban_id) {
    return dispatch([this, ban_id]() -> bool {
        if (!db_)
            return false;

        auto check = prepare("SELECT ID FROM bans WHERE ID = ?");
        if (!check)
            return false;
        sqlite3_bind_int64(check.get(), 1, ban_id);
        if (sqlite3_step(check.get()) != SQLITE_ROW)
            return false;

        auto stmt = prepare("UPDATE bans SET DURATION = 0 WHERE ID = ?");
        if (!stmt)
            return false;
        sqlite3_bind_int64(stmt.get(), 1, ban_id);

        return sqlite3_step(stmt.get()) == SQLITE_DONE;
    });
}

std::future<bool> DatabaseManager::update_ban(int64_t ban_id, std::string field, std::string value) {
    return dispatch([this, ban_id, field = std::move(field), value = std::move(value)]() -> bool {
        if (field != "reason" && field != "duration")
            return false;
        if (!db_)
            return false;

        std::string sql = "UPDATE bans SET " + field + " = ? WHERE ID = ?";
        auto stmt = prepare(sql.c_str());
        if (!stmt)
            return false;

        if (field == "duration") {
            int64_t dur = 0;
            try {
                dur = std::stoll(value);
            }
            catch (...) {
                return false;
            }
            sqlite3_bind_int64(stmt.get(), 1, dur);
        }
        else {
            sqlite3_bind_text(stmt.get(), 1, value.c_str(), -1, SQLITE_TRANSIENT);
        }

        sqlite3_bind_int64(stmt.get(), 2, ban_id);
        return sqlite3_step(stmt.get()) == SQLITE_DONE;
    });
}

std::future<std::vector<BanEntry>> DatabaseManager::recent_bans(int limit) {
    return dispatch([this, limit]() -> std::vector<BanEntry> {
        std::vector<BanEntry> result;
        if (!db_)
            return result;

        auto stmt = prepare("SELECT * FROM bans ORDER BY TIME DESC LIMIT ?");
        if (!stmt)
            return result;

        sqlite3_bind_int(stmt.get(), 1, limit);

        while (sqlite3_step(stmt.get()) == SQLITE_ROW)
            result.push_back(row_to_ban(stmt.get()));

        return result;
    });
}

std::future<std::vector<BanEntry>> DatabaseManager::all_active_bans() {
    return dispatch([this]() -> std::vector<BanEntry> {
        std::vector<BanEntry> result;
        if (!db_)
            return result;

        auto stmt = prepare("SELECT * FROM bans");
        if (!stmt)
            return result;

        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            auto entry = row_to_ban(stmt.get());
            if (is_ban_active(entry))
                result.push_back(std::move(entry));
        }

        return result;
    });
}

// -- User operations ----------------------------------------------------------

std::future<bool> DatabaseManager::create_user(std::string username, std::string salt, std::string password_hash,
                                               std::string acl) {
    return dispatch([this, username = std::move(username), salt = std::move(salt),
                     password_hash = std::move(password_hash), acl = std::move(acl)]() -> bool {
        if (!db_)
            return false;

        auto check = prepare("SELECT 1 FROM users WHERE USERNAME = ?");
        if (!check)
            return false;
        sqlite3_bind_text(check.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(check.get()) == SQLITE_ROW)
            return false;

        auto stmt = prepare("INSERT INTO users (USERNAME, SALT, PASSWORD, ACL) VALUES (?, ?, ?, ?)");
        if (!stmt)
            return false;

        sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, salt.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 3, password_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 4, acl.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            Log::log_print(WARNING, "DatabaseManager: create_user failed: %s", sqlite3_errmsg(db_));
            return false;
        }

        return true;
    });
}

std::future<bool> DatabaseManager::delete_user(std::string username) {
    return dispatch([this, username = std::move(username)]() -> bool {
        if (!db_)
            return false;

        auto stmt = prepare("DELETE FROM users WHERE USERNAME = ?");
        if (!stmt)
            return false;

        sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE)
            return false;

        return sqlite3_changes(db_) > 0;
    });
}

std::future<std::optional<UserEntry>> DatabaseManager::get_user(std::string username) {
    return dispatch([this, username = std::move(username)]() -> std::optional<UserEntry> {
        if (!db_)
            return std::nullopt;

        auto stmt = prepare("SELECT * FROM users WHERE USERNAME = ?");
        if (!stmt)
            return std::nullopt;

        sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt.get()) != SQLITE_ROW)
            return std::nullopt;

        auto col_text = [&](int col) -> std::string {
            auto* p = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), col));
            return p ? p : "";
        };

        UserEntry user;
        user.id = sqlite3_column_int64(stmt.get(), 0);
        user.username = col_text(1);
        user.salt = col_text(2);
        user.password = col_text(3);
        user.acl = col_text(4);

        return user;
    });
}

std::future<std::vector<std::string>> DatabaseManager::list_users() {
    return dispatch([this]() -> std::vector<std::string> {
        std::vector<std::string> result;
        if (!db_)
            return result;

        auto stmt = prepare("SELECT USERNAME FROM users ORDER BY ID");
        if (!stmt)
            return result;

        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0));
            if (text)
                result.emplace_back(text);
        }

        return result;
    });
}

std::future<bool> DatabaseManager::update_acl(std::string username, std::string acl) {
    return dispatch([this, username = std::move(username), acl = std::move(acl)]() -> bool {
        if (!db_)
            return false;

        auto stmt = prepare("UPDATE users SET ACL = ? WHERE USERNAME = ?");
        if (!stmt)
            return false;

        sqlite3_bind_text(stmt.get(), 1, acl.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, username.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE)
            return false;

        return sqlite3_changes(db_) > 0;
    });
}

std::future<bool> DatabaseManager::update_password(std::string username, std::string salt, std::string password_hash) {
    return dispatch([this, username = std::move(username), salt = std::move(salt),
                     password_hash = std::move(password_hash)]() -> bool {
        if (!db_)
            return false;

        auto stmt = prepare("UPDATE users SET SALT = ?, PASSWORD = ? WHERE USERNAME = ?");
        if (!stmt)
            return false;

        sqlite3_bind_text(stmt.get(), 1, salt.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 3, username.c_str(), -1, SQLITE_TRANSIENT);

        return sqlite3_step(stmt.get()) == SQLITE_DONE;
    });
}

// -- Internal helpers (worker thread only) ------------------------------------

bool DatabaseManager::exec(const char* sql) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        Log::log_print(WARNING, "DatabaseManager: exec failed: %s (SQL: %s)", errmsg ? errmsg : "unknown", sql);
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

PreparedStatement DatabaseManager::prepare(const char* sql) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        Log::log_print(WARNING, "DatabaseManager: prepare failed: %s (SQL: %s)", sqlite3_errmsg(db_), sql);
        return {};
    }
    return PreparedStatement(stmt);
}

BanEntry DatabaseManager::row_to_ban(sqlite3_stmt* stmt) {
    BanEntry entry;
    auto col_text = [&](int col) -> std::string {
        auto* p = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
        return p ? p : "";
    };

    entry.id = sqlite3_column_int64(stmt, 0);
    entry.ipid = col_text(1);
    entry.hdid = col_text(2);
    entry.ip = col_text(3);
    entry.timestamp = sqlite3_column_int64(stmt, 4);
    entry.reason = col_text(5);
    entry.duration = sqlite3_column_int64(stmt, 6);
    entry.moderator = col_text(7);

    return entry;
}

bool DatabaseManager::is_ban_active(const BanEntry& entry) {
    if (entry.duration == 0)
        return false;
    return !entry.is_expired();
}

// -- Moderation events --------------------------------------------------------

std::future<int64_t> DatabaseManager::record_moderation_event(moderation::ModerationEvent event) {
    return dispatch([this, event = std::move(event)]() -> int64_t {
        if (!db_)
            return -1;

        auto stmt = prepare("INSERT INTO moderation_events ("
                            "  TIMESTAMP_MS, IPID, CHANNEL, MESSAGE_SAMPLE, ACTION, HEAT_AFTER, REASON,"
                            "  VISUAL_NOISE, LINK_RISK, HATE, SEXUAL, SEXUAL_MINORS,"
                            "  VIOLENCE, SELF_HARM, SEMANTIC_ECHO"
                            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        if (!stmt)
            return -1;

        sqlite3_bind_int64(stmt.get(), 1, event.timestamp_ms);
        sqlite3_bind_text(stmt.get(), 2, event.ipid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 3, event.channel.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 4, event.message_sample.c_str(), -1, SQLITE_TRANSIENT);
        const char* action_str = moderation::to_string(event.action);
        sqlite3_bind_text(stmt.get(), 5, action_str, -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt.get(), 6, event.heat_after);
        sqlite3_bind_text(stmt.get(), 7, event.reason.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt.get(), 8, event.scores.visual_noise);
        sqlite3_bind_double(stmt.get(), 9, event.scores.link_risk);
        sqlite3_bind_double(stmt.get(), 10, event.scores.hate);
        sqlite3_bind_double(stmt.get(), 11, event.scores.sexual);
        sqlite3_bind_double(stmt.get(), 12, event.scores.sexual_minors);
        sqlite3_bind_double(stmt.get(), 13, event.scores.violence);
        sqlite3_bind_double(stmt.get(), 14, event.scores.self_harm);
        sqlite3_bind_double(stmt.get(), 15, event.scores.semantic_echo);

        if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
            Log::log_print(WARNING, "DatabaseManager: record_moderation_event failed: %s", sqlite3_errmsg(db_));
            return -1;
        }
        return sqlite3_last_insert_rowid(db_);
    });
}

std::future<std::vector<moderation::ModerationEvent>> DatabaseManager::query_moderation_events(ModerationQuery query) {
    return dispatch([this, query = std::move(query)]() -> std::vector<moderation::ModerationEvent> {
        std::vector<moderation::ModerationEvent> result;
        if (!db_)
            return result;

        // Build a dynamic WHERE clause. We only add filters that were
        // actually set, so the common "recent events" query hits the
        // primary timestamp index cleanly.
        std::string sql = "SELECT ID, TIMESTAMP_MS, IPID, CHANNEL, MESSAGE_SAMPLE, ACTION, HEAT_AFTER, REASON,"
                          " VISUAL_NOISE, LINK_RISK, HATE, SEXUAL, SEXUAL_MINORS,"
                          " VIOLENCE, SELF_HARM, SEMANTIC_ECHO FROM moderation_events";
        std::vector<std::string> clauses;
        if (query.ipid)
            clauses.emplace_back("IPID = ?");
        if (query.channel)
            clauses.emplace_back("CHANNEL = ?");
        if (query.action)
            clauses.emplace_back("ACTION = ?");
        if (query.since_ms)
            clauses.emplace_back("TIMESTAMP_MS >= ?");
        if (query.until_ms)
            clauses.emplace_back("TIMESTAMP_MS <= ?");
        if (!clauses.empty()) {
            sql += " WHERE ";
            for (size_t i = 0; i < clauses.size(); ++i) {
                if (i > 0)
                    sql += " AND ";
                sql += clauses[i];
            }
        }
        sql += " ORDER BY TIMESTAMP_MS DESC LIMIT ?";

        auto stmt = prepare(sql.c_str());
        if (!stmt)
            return result;

        int bind = 1;
        if (query.ipid)
            sqlite3_bind_text(stmt.get(), bind++, query.ipid->c_str(), -1, SQLITE_TRANSIENT);
        if (query.channel)
            sqlite3_bind_text(stmt.get(), bind++, query.channel->c_str(), -1, SQLITE_TRANSIENT);
        if (query.action)
            sqlite3_bind_text(stmt.get(), bind++, query.action->c_str(), -1, SQLITE_TRANSIENT);
        if (query.since_ms)
            sqlite3_bind_int64(stmt.get(), bind++, *query.since_ms);
        if (query.until_ms)
            sqlite3_bind_int64(stmt.get(), bind++, *query.until_ms);
        const int limit = std::clamp(query.limit, 1, 1000);
        sqlite3_bind_int(stmt.get(), bind++, limit);

        auto col_text = [&](int col) -> std::string {
            auto* p = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), col));
            return p ? p : "";
        };
        auto parse_action = [](const std::string& s) -> moderation::ModerationAction {
            using A = moderation::ModerationAction;
            if (s == "log")
                return A::LOG;
            if (s == "censor")
                return A::CENSOR;
            if (s == "drop")
                return A::DROP;
            if (s == "mute")
                return A::MUTE;
            if (s == "kick")
                return A::KICK;
            if (s == "ban")
                return A::BAN;
            if (s == "perma_ban")
                return A::PERMA_BAN;
            return A::NONE;
        };

        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            moderation::ModerationEvent ev;
            ev.id = sqlite3_column_int64(stmt.get(), 0);
            ev.timestamp_ms = sqlite3_column_int64(stmt.get(), 1);
            ev.ipid = col_text(2);
            ev.channel = col_text(3);
            ev.message_sample = col_text(4);
            ev.action = parse_action(col_text(5));
            ev.heat_after = sqlite3_column_double(stmt.get(), 6);
            ev.reason = col_text(7);
            ev.scores.visual_noise = sqlite3_column_double(stmt.get(), 8);
            ev.scores.link_risk = sqlite3_column_double(stmt.get(), 9);
            ev.scores.hate = sqlite3_column_double(stmt.get(), 10);
            ev.scores.sexual = sqlite3_column_double(stmt.get(), 11);
            ev.scores.sexual_minors = sqlite3_column_double(stmt.get(), 12);
            ev.scores.violence = sqlite3_column_double(stmt.get(), 13);
            ev.scores.self_harm = sqlite3_column_double(stmt.get(), 14);
            ev.scores.semantic_echo = sqlite3_column_double(stmt.get(), 15);
            result.push_back(std::move(ev));
        }
        return result;
    });
}

// -- Mutes --------------------------------------------------------------------

std::future<int64_t> DatabaseManager::add_mute(MuteEntry entry) {
    return dispatch([this, entry = std::move(entry)]() -> int64_t {
        if (!db_)
            return -1;
        auto stmt = prepare("INSERT INTO mutes (IPID, STARTED_AT, EXPIRES_AT, REASON, MODERATOR) "
                            "VALUES (?, ?, ?, ?, ?)");
        if (!stmt)
            return -1;
        sqlite3_bind_text(stmt.get(), 1, entry.ipid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt.get(), 2, entry.started_at);
        sqlite3_bind_int64(stmt.get(), 3, entry.expires_at);
        sqlite3_bind_text(stmt.get(), 4, entry.reason.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 5, entry.moderator.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt.get()) != SQLITE_DONE)
            return -1;
        return sqlite3_last_insert_rowid(db_);
    });
}

std::future<std::vector<MuteEntry>> DatabaseManager::active_mutes() {
    return dispatch([this]() -> std::vector<MuteEntry> {
        std::vector<MuteEntry> result;
        if (!db_)
            return result;
        const int64_t now =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        auto stmt = prepare("SELECT ID, IPID, STARTED_AT, EXPIRES_AT, REASON, MODERATOR FROM mutes "
                            "WHERE EXPIRES_AT = 0 OR EXPIRES_AT > ?");
        if (!stmt)
            return result;
        sqlite3_bind_int64(stmt.get(), 1, now);
        auto col_text = [&](int col) -> std::string {
            auto* p = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), col));
            return p ? p : "";
        };
        while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
            MuteEntry m;
            m.id = sqlite3_column_int64(stmt.get(), 0);
            m.ipid = col_text(1);
            m.started_at = sqlite3_column_int64(stmt.get(), 2);
            m.expires_at = sqlite3_column_int64(stmt.get(), 3);
            m.reason = col_text(4);
            m.moderator = col_text(5);
            result.push_back(std::move(m));
        }
        return result;
    });
}

std::future<int> DatabaseManager::delete_mutes_by_ipid(std::string ipid) {
    return dispatch([this, ipid = std::move(ipid)]() -> int {
        if (!db_)
            return 0;
        auto stmt = prepare("DELETE FROM mutes WHERE IPID = ?");
        if (!stmt)
            return 0;
        sqlite3_bind_text(stmt.get(), 1, ipid.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt.get()) != SQLITE_DONE)
            return 0;
        return sqlite3_changes(db_);
    });
}

std::future<int> DatabaseManager::prune_expired_mutes() {
    return dispatch([this]() -> int {
        if (!db_)
            return 0;
        const int64_t now =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        auto stmt = prepare("DELETE FROM mutes WHERE EXPIRES_AT > 0 AND EXPIRES_AT <= ?");
        if (!stmt)
            return 0;
        sqlite3_bind_int64(stmt.get(), 1, now);
        if (sqlite3_step(stmt.get()) != SQLITE_DONE)
            return 0;
        return sqlite3_changes(db_);
    });
}

// -- Auth token operations ----------------------------------------------------

std::future<bool> DatabaseManager::create_auth_token(AuthTokenEntry entry) {
    return dispatch([this, entry = std::move(entry)]() -> bool {
        if (!db_)
            return false;
        auto stmt = prepare("INSERT INTO auth_tokens (TOKEN, USERNAME, ACL, CREATED_AT, EXPIRES_AT, DESCRIPTION) "
                            "VALUES (?, ?, ?, ?, ?, ?)");
        if (!stmt)
            return false;
        sqlite3_bind_text(stmt.get(), 1, entry.token.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 2, entry.username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt.get(), 3, entry.acl.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt.get(), 4, entry.created_at);
        sqlite3_bind_int64(stmt.get(), 5, entry.expires_at);
        sqlite3_bind_text(stmt.get(), 6, entry.description.c_str(), -1, SQLITE_TRANSIENT);
        return sqlite3_step(stmt.get()) == SQLITE_DONE;
    });
}

std::future<std::optional<AuthTokenEntry>> DatabaseManager::validate_auth_token(std::string token) {
    return dispatch([this, token = std::move(token)]() -> std::optional<AuthTokenEntry> {
        if (!db_)
            return std::nullopt;
        auto stmt = prepare("SELECT ID, TOKEN, USERNAME, ACL, CREATED_AT, EXPIRES_AT, REVOKED, LAST_USED, DESCRIPTION "
                            "FROM auth_tokens WHERE TOKEN = ?");
        if (!stmt)
            return std::nullopt;
        sqlite3_bind_text(stmt.get(), 1, token.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt.get()) != SQLITE_ROW)
            return std::nullopt;

        AuthTokenEntry entry;
        entry.id = sqlite3_column_int64(stmt.get(), 0);
        entry.token = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        entry.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        entry.acl = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
        entry.created_at = sqlite3_column_int64(stmt.get(), 4);
        entry.expires_at = sqlite3_column_int64(stmt.get(), 5);
        entry.revoked = sqlite3_column_int(stmt.get(), 6) != 0;
        entry.last_used = sqlite3_column_int64(stmt.get(), 7);
        auto* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 8));
        entry.description = desc ? desc : "";

        // Check revocation
        if (entry.revoked)
            return std::nullopt;

        // Check expiry
        if (entry.expires_at > 0) {
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
            if (now >= entry.expires_at)
                return std::nullopt;
        }

        return entry;
    });
}

std::future<bool> DatabaseManager::revoke_auth_token(std::string token) {
    return dispatch([this, token = std::move(token)]() -> bool {
        if (!db_)
            return false;
        auto stmt = prepare("UPDATE auth_tokens SET REVOKED = 1 WHERE TOKEN = ? AND REVOKED = 0");
        if (!stmt)
            return false;
        sqlite3_bind_text(stmt.get(), 1, token.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt.get()) != SQLITE_DONE)
            return false;
        return sqlite3_changes(db_) > 0;
    });
}

std::future<int> DatabaseManager::revoke_all_tokens_for_user(std::string username) {
    return dispatch([this, username = std::move(username)]() -> int {
        if (!db_)
            return 0;
        auto stmt = prepare("UPDATE auth_tokens SET REVOKED = 1 WHERE USERNAME = ? AND REVOKED = 0");
        if (!stmt)
            return 0;
        sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt.get()) != SQLITE_DONE)
            return 0;
        return sqlite3_changes(db_);
    });
}

std::future<void> DatabaseManager::touch_auth_token(std::string token) {
    return dispatch([this, token = std::move(token)]() {
        if (!db_)
            return;
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
        auto stmt = prepare("UPDATE auth_tokens SET LAST_USED = ? WHERE TOKEN = ?");
        if (!stmt)
            return;
        sqlite3_bind_int64(stmt.get(), 1, now);
        sqlite3_bind_text(stmt.get(), 2, token.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt.get());
    });
}
