/**
 * @file DatabaseManager.h
 * @brief SQLite-backed persistent storage for bans and users.
 *
 * Schema-compatible with akashi (same table layout), using the SQLite3
 * amalgamation vendored in third-party/sqlite3/.
 *
 * Thread safety: all DB operations are dispatched to a single background
 * worker thread via a task queue. Public methods return std::future<T>
 * so callers can await results without blocking the server's hot paths.
 * The sqlite3 connection is only ever accessed from the worker thread.
 */
#pragma once

#include "game/BanManager.h" // BanEntry

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

/// Default database file path (next to the kagami binary).
inline constexpr const char* DEFAULT_DB_FILE = "kagami.db";

/// Current schema version. Bump when adding migrations.
inline constexpr int DB_VERSION = 1;

/// A row from the `users` table.
struct UserEntry {
    int64_t id = 0;
    std::string username;
    std::string salt;     ///< Hex-encoded salt.
    std::string password; ///< Hashed password (hex).
    std::string acl;      ///< Role identifier (e.g. "SUPER", "MOD", "NONE").
};

/// RAII wrapper for sqlite3_stmt that finalises on destruction.
class PreparedStatement {
  public:
    PreparedStatement() = default;
    explicit PreparedStatement(sqlite3_stmt* s) : stmt_(s) {
    }
    ~PreparedStatement();

    PreparedStatement(PreparedStatement&& o) noexcept : stmt_(o.stmt_) {
        o.stmt_ = nullptr;
    }
    PreparedStatement& operator=(PreparedStatement&& o) noexcept;

    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;

    sqlite3_stmt* get() const {
        return stmt_;
    }
    explicit operator bool() const {
        return stmt_ != nullptr;
    }

  private:
    sqlite3_stmt* stmt_ = nullptr;
};

class DatabaseManager {
  public:
    DatabaseManager();
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /// Open (or create) the database at @p path.
    /// Creates tables and runs migrations if needed.
    /// Blocks until the worker thread has completed initialisation.
    /// Returns true on success.
    bool open(const std::string& path);

    /// Close the database. Drains the work queue and joins the worker thread.
    void close();

    /// True if the database was opened successfully.
    bool is_open() const;

    // -- Ban operations -------------------------------------------------------

    /// Insert a ban record. Future resolves to the new row ID, or -1 on failure.
    std::future<int64_t> add_ban(BanEntry entry);

    /// Look up an active ban by IPID.
    std::future<std::optional<BanEntry>> find_ban_by_ipid(std::string ipid);

    /// Look up an active ban by HDID.
    std::future<std::optional<BanEntry>> find_ban_by_hdid(std::string hdid);

    /// Check if an IPID or HDID is banned.
    std::future<std::optional<BanEntry>> check_ban(std::string ipid, std::string hdid);

    /// Invalidate (soft-delete) a ban by setting its duration to 0.
    std::future<bool> invalidate_ban(int64_t ban_id);

    /// Update a field on a ban (only "reason" or "duration" are allowed).
    std::future<bool> update_ban(int64_t ban_id, std::string field, std::string value);

    /// Return the N most recent bans.
    std::future<std::vector<BanEntry>> recent_bans(int limit = 5);

    // -- User operations ------------------------------------------------------

    /// Create a new user. Future resolves to true on success, false if username exists.
    std::future<bool> create_user(std::string username, std::string salt, std::string password_hash, std::string acl);

    /// Delete a user by username. Future resolves to true if the user was deleted.
    std::future<bool> delete_user(std::string username);

    /// Retrieve a user record by username.
    std::future<std::optional<UserEntry>> get_user(std::string username);

    /// List all usernames.
    std::future<std::vector<std::string>> list_users();

    /// Update the ACL role for a user.
    std::future<bool> update_acl(std::string username, std::string acl);

    /// Update the password (and salt) for a user.
    std::future<bool> update_password(std::string username, std::string salt, std::string password_hash);

  private:
    sqlite3* db_ = nullptr;         ///< Only accessed from worker thread.
    std::atomic<bool> open_{false}; ///< Set after successful open().

    // -- Worker thread and task queue -----------------------------------------
    std::jthread worker_;
    std::mutex queue_mutex_;
    std::condition_variable_any queue_cv_;
    std::deque<std::function<void()>> queue_;

    void worker_loop(std::stop_token stop);

    /// Enqueue a task for the worker thread.
    void enqueue(std::function<void()> task);

    /// Helper: enqueue a callable and return a future for its result.
    template <typename F>
    auto dispatch(F&& fn) -> std::future<decltype(fn())>;

    // -- DB helpers (called only from worker thread) --------------------------

    /// Create tables if they don't already exist.
    bool create_tables();

    /// Read the PRAGMA user_version and run any pending migrations.
    bool migrate();

    /// Run a single SQL statement (no result). Returns true on success.
    bool exec(const char* sql);

    /// Prepare a statement. Returns an empty PreparedStatement on failure.
    PreparedStatement prepare(const char* sql);

    /// Extract a BanEntry from a SELECT * FROM bans query at the current row.
    static BanEntry row_to_ban(sqlite3_stmt* stmt);

    /// Check if a ban row is currently active (not expired).
    static bool is_ban_active(const BanEntry& entry);

    // -- Inline query helpers (worker thread only) ----------------------------
    std::optional<BanEntry> do_find_ban_by_ipid(const std::string& ipid);
    std::optional<BanEntry> do_find_ban_by_hdid(const std::string& hdid);
};

// -- Template implementation --------------------------------------------------

template <typename F>
auto DatabaseManager::dispatch(F&& fn) -> std::future<decltype(fn())> {
    using R = decltype(fn());
    auto promise = std::make_shared<std::promise<R>>();
    auto future = promise->get_future();
    enqueue([p = std::move(promise), f = std::forward<F>(fn)]() mutable {
        try {
            if constexpr (std::is_void_v<R>) {
                f();
                p->set_value();
            }
            else {
                p->set_value(f());
            }
        }
        catch (...) {
            p->set_exception(std::current_exception());
        }
    });
    return future;
}
