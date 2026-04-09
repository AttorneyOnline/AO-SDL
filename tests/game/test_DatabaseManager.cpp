#include <gtest/gtest.h>

#include "game/DatabaseManager.h"

#include <cstdio>
#include <filesystem>
#include <string>

namespace {

class DatabaseManagerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Use a unique temp file per test to avoid interference.
        db_path_ = std::filesystem::temp_directory_path() / ("test_kagami_" + std::to_string(counter_++) + ".db");
        ASSERT_TRUE(db_.open(db_path_.string()));
    }

    void TearDown() override {
        db_.close();
        // Clean up temp files (db + WAL/shm).
        std::filesystem::remove(db_path_);
        std::filesystem::remove(db_path_.string() + "-wal");
        std::filesystem::remove(db_path_.string() + "-shm");
    }

    DatabaseManager db_;
    std::filesystem::path db_path_;
    static int counter_;
};

int DatabaseManagerTest::counter_ = 0;

} // namespace

// -- Open / Close -------------------------------------------------------------

TEST_F(DatabaseManagerTest, OpenCreatesFile) {
    EXPECT_TRUE(db_.is_open());
    EXPECT_TRUE(std::filesystem::exists(db_path_));
}

TEST_F(DatabaseManagerTest, CloseAndReopen) {
    db_.close();
    EXPECT_FALSE(db_.is_open());
    EXPECT_TRUE(db_.open(db_path_.string()));
    EXPECT_TRUE(db_.is_open());
}

TEST_F(DatabaseManagerTest, DoubleCloseIsSafe) {
    db_.close();
    db_.close(); // Should not crash.
    EXPECT_FALSE(db_.is_open());
}

// -- Ban CRUD -----------------------------------------------------------------

TEST_F(DatabaseManagerTest, AddBan) {
    BanEntry ban;
    ban.ipid = "abc123";
    ban.hdid = "hw456";
    ban.reason = "testing";
    ban.moderator = "admin";
    ban.duration = -2; // permanent

    auto id = db_.add_ban(ban).get();
    EXPECT_GT(id, 0);
}

TEST_F(DatabaseManagerTest, FindBanByIpid) {
    BanEntry ban;
    ban.ipid = "findme";
    ban.hdid = "hw";
    ban.reason = "test";
    ban.duration = -2;
    db_.add_ban(ban).get();

    auto found = db_.find_ban_by_ipid("findme").get();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->ipid, "findme");
    EXPECT_EQ(found->reason, "test");
    EXPECT_TRUE(found->is_permanent());
}

TEST_F(DatabaseManagerTest, FindBanByHdid) {
    BanEntry ban;
    ban.ipid = "ip1";
    ban.hdid = "unique_hw";
    ban.reason = "hw ban";
    ban.duration = -2;
    db_.add_ban(ban).get();

    auto found = db_.find_ban_by_hdid("unique_hw").get();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->hdid, "unique_hw");
}

TEST_F(DatabaseManagerTest, FindBanByHdid_Empty) {
    auto found = db_.find_ban_by_hdid("").get();
    EXPECT_FALSE(found.has_value());
}

TEST_F(DatabaseManagerTest, CheckBan_MatchesIpid) {
    BanEntry ban;
    ban.ipid = "banned_ip";
    ban.hdid = "banned_hw";
    ban.duration = -2;
    db_.add_ban(ban).get();

    auto found = db_.check_ban("banned_ip", "other_hw").get();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->ipid, "banned_ip");
}

TEST_F(DatabaseManagerTest, CheckBan_MatchesHdid) {
    BanEntry ban;
    ban.ipid = "some_ip";
    ban.hdid = "banned_hw";
    ban.duration = -2;
    db_.add_ban(ban).get();

    auto found = db_.check_ban("other_ip", "banned_hw").get();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->hdid, "banned_hw");
}

TEST_F(DatabaseManagerTest, CheckBan_NoBan) {
    auto found = db_.check_ban("clean_ip", "clean_hw").get();
    EXPECT_FALSE(found.has_value());
}

TEST_F(DatabaseManagerTest, InvalidateBan) {
    BanEntry ban;
    ban.ipid = "to_unban";
    ban.duration = -2;
    auto id = db_.add_ban(ban).get();

    EXPECT_TRUE(db_.invalidate_ban(id).get());

    // Should no longer be found as active.
    auto found = db_.find_ban_by_ipid("to_unban").get();
    EXPECT_FALSE(found.has_value());
}

TEST_F(DatabaseManagerTest, InvalidateBan_NonExistent) {
    EXPECT_FALSE(db_.invalidate_ban(99999).get());
}

TEST_F(DatabaseManagerTest, UpdateBan_Reason) {
    BanEntry ban;
    ban.ipid = "update_test";
    ban.reason = "original";
    ban.duration = -2;
    auto id = db_.add_ban(ban).get();

    EXPECT_TRUE(db_.update_ban(id, "reason", "updated reason").get());

    auto found = db_.find_ban_by_ipid("update_test").get();
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->reason, "updated reason");
}

TEST_F(DatabaseManagerTest, UpdateBan_InvalidField) {
    BanEntry ban;
    ban.ipid = "x";
    ban.duration = -2;
    auto id = db_.add_ban(ban).get();

    EXPECT_FALSE(db_.update_ban(id, "ipid", "hacked").get()); // only reason/duration allowed
}

TEST_F(DatabaseManagerTest, RecentBans) {
    for (int i = 0; i < 10; i++) {
        BanEntry ban;
        ban.ipid = "ban" + std::to_string(i);
        ban.duration = -2;
        ban.timestamp = 1000 + i;
        db_.add_ban(ban).get();
    }

    auto recent = db_.recent_bans(3).get();
    EXPECT_EQ(recent.size(), 3u);
    // Most recent first (highest timestamp).
    EXPECT_EQ(recent[0].ipid, "ban9");
    EXPECT_EQ(recent[1].ipid, "ban8");
    EXPECT_EQ(recent[2].ipid, "ban7");
}

TEST_F(DatabaseManagerTest, ExpiredBanNotFound) {
    BanEntry ban;
    ban.ipid = "expired";
    ban.timestamp = 1; // epoch + 1s
    ban.duration = 1;  // 1 second duration → expired long ago
    db_.add_ban(ban).get();

    auto found = db_.find_ban_by_ipid("expired").get();
    EXPECT_FALSE(found.has_value());
}

// -- User CRUD ----------------------------------------------------------------

TEST_F(DatabaseManagerTest, CreateUser) {
    EXPECT_TRUE(db_.create_user("alice", "salt", "hash", "SUPER").get());
}

TEST_F(DatabaseManagerTest, CreateUser_Duplicate) {
    EXPECT_TRUE(db_.create_user("alice", "salt", "hash", "SUPER").get());
    EXPECT_FALSE(db_.create_user("alice", "salt2", "hash2", "NONE").get());
}

TEST_F(DatabaseManagerTest, GetUser) {
    db_.create_user("bob", "aabb", "ccdd", "MOD").get();

    auto user = db_.get_user("bob").get();
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->username, "bob");
    EXPECT_EQ(user->salt, "aabb");
    EXPECT_EQ(user->password, "ccdd");
    EXPECT_EQ(user->acl, "MOD");
}

TEST_F(DatabaseManagerTest, GetUser_NotFound) {
    auto user = db_.get_user("nobody").get();
    EXPECT_FALSE(user.has_value());
}

TEST_F(DatabaseManagerTest, DeleteUser) {
    db_.create_user("victim", "s", "p", "NONE").get();
    EXPECT_TRUE(db_.delete_user("victim").get());
    EXPECT_FALSE(db_.get_user("victim").get().has_value());
}

TEST_F(DatabaseManagerTest, DeleteUser_NotFound) {
    EXPECT_FALSE(db_.delete_user("ghost").get());
}

TEST_F(DatabaseManagerTest, ListUsers) {
    db_.create_user("charlie", "s", "p", "NONE").get();
    db_.create_user("alice", "s", "p", "SUPER").get();
    db_.create_user("bob", "s", "p", "MOD").get();

    auto users = db_.list_users().get();
    EXPECT_EQ(users.size(), 3u);
    // Ordered by ID (insertion order).
    EXPECT_EQ(users[0], "charlie");
    EXPECT_EQ(users[1], "alice");
    EXPECT_EQ(users[2], "bob");
}

TEST_F(DatabaseManagerTest, ListUsers_Empty) {
    auto users = db_.list_users().get();
    EXPECT_TRUE(users.empty());
}

TEST_F(DatabaseManagerTest, UpdateACL) {
    db_.create_user("eve", "s", "p", "NONE").get();
    EXPECT_TRUE(db_.update_acl("eve", "SUPER").get());

    auto user = db_.get_user("eve").get();
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->acl, "SUPER");
}

TEST_F(DatabaseManagerTest, UpdateACL_NotFound) {
    EXPECT_FALSE(db_.update_acl("ghost", "SUPER").get());
}

TEST_F(DatabaseManagerTest, UpdatePassword) {
    db_.create_user("frank", "oldsalt", "oldhash", "NONE").get();
    EXPECT_TRUE(db_.update_password("frank", "newsalt", "newhash").get());

    auto user = db_.get_user("frank").get();
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->salt, "newsalt");
    EXPECT_EQ(user->password, "newhash");
}

// -- Schema versioning --------------------------------------------------------

TEST_F(DatabaseManagerTest, ReopenPreservesData) {
    db_.create_user("persist", "s", "p", "SUPER").get();
    BanEntry ban;
    ban.ipid = "persistban";
    ban.duration = -2;
    db_.add_ban(ban).get();

    // Close and reopen.
    db_.close();
    ASSERT_TRUE(db_.open(db_path_.string()));

    auto user = db_.get_user("persist").get();
    ASSERT_TRUE(user.has_value());
    EXPECT_EQ(user->username, "persist");

    auto found = db_.find_ban_by_ipid("persistban").get();
    ASSERT_TRUE(found.has_value());
}

// -- Async dispatch -----------------------------------------------------------

TEST_F(DatabaseManagerTest, ConcurrentOperations) {
    // Launch several async operations and verify they all complete.
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < 20; i++) {
        futures.push_back(db_.create_user("user" + std::to_string(i), "s", "p", "NONE"));
    }

    for (auto& f : futures)
        EXPECT_TRUE(f.get());

    auto users = db_.list_users().get();
    EXPECT_EQ(users.size(), 20u);
}
