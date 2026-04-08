#include "game/DatabaseManager.h"

#include "utils/Log.h"

#include <sqlite3.h>

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

    switch (version) {
    case 0:
        if (!exec("PRAGMA user_version = 1"))
            return false;
        [[fallthrough]];
    default:
        break;
    }

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
        sqlite3_bind_text(stmt.get(), 3, "", -1, SQLITE_STATIC);
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

        UserEntry user;
        user.id = sqlite3_column_int64(stmt.get(), 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
        user.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
        user.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
        user.acl = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4));

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

    entry.ipid = col_text(1);
    entry.hdid = col_text(2);
    // column 3 = IP (not in BanEntry)
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
