#include "utils/Log.h"

#include <gtest/gtest.h>
#include <mutex>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: RAII guard that installs a log sink and removes it on destruction.
// ---------------------------------------------------------------------------

struct CapturedMessage {
    LogLevel level;
    std::string timestamp;
    std::string message;
};

class LogSinkGuard {
  public:
    LogSinkGuard() {
        Log::set_sink([this](LogLevel level, const std::string& ts, const std::string& msg) {
            std::lock_guard lock(mu_);
            captured_.push_back({level, ts, msg});
        });
    }

    ~LogSinkGuard() {
        Log::set_sink(nullptr);
    }

    std::vector<CapturedMessage> drain() {
        std::lock_guard lock(mu_);
        auto copy = captured_;
        captured_.clear();
        return copy;
    }

    size_t count() {
        std::lock_guard lock(mu_);
        return captured_.size();
    }

  private:
    std::mutex mu_;
    std::vector<CapturedMessage> captured_;
};

// ---------------------------------------------------------------------------
// log_level_name
// ---------------------------------------------------------------------------

TEST(LogLevelName, ReturnsCorrectStrings) {
    EXPECT_STREQ(log_level_name(VERBOSE), "VERBOSE");
    EXPECT_STREQ(log_level_name(DEBUG),   "DEBUG");
    EXPECT_STREQ(log_level_name(INFO),    "INFO");
    EXPECT_STREQ(log_level_name(WARNING), "WARNING");
    EXPECT_STREQ(log_level_name(ERR),     "ERROR");
    EXPECT_STREQ(log_level_name(FATAL),   "FATAL");
}

TEST(LogLevelName, InvalidLevelReturnsUnknown) {
    EXPECT_STREQ(log_level_name(INVALID), "UNKNOWN");
    EXPECT_STREQ(log_level_name(COUNT),   "UNKNOWN");
    EXPECT_STREQ(log_level_name(static_cast<LogLevel>(-1)), "UNKNOWN");
    EXPECT_STREQ(log_level_name(static_cast<LogLevel>(99)), "UNKNOWN");
}

// ---------------------------------------------------------------------------
// log_print with sink — severity levels
// ---------------------------------------------------------------------------

TEST(LogPrint, VerboseLevel) {
    LogSinkGuard guard;
    Log::log_print(VERBOSE, "verbose message");
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].level, VERBOSE);
    EXPECT_EQ(msgs[0].message, "verbose message");
}

TEST(LogPrint, DebugLevel) {
    LogSinkGuard guard;
    Log::log_print(DEBUG, "debug message");
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].level, DEBUG);
    EXPECT_EQ(msgs[0].message, "debug message");
}

TEST(LogPrint, InfoLevel) {
    LogSinkGuard guard;
    Log::log_print(INFO, "info message");
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].level, INFO);
    EXPECT_EQ(msgs[0].message, "info message");
}

TEST(LogPrint, WarningLevel) {
    LogSinkGuard guard;
    Log::log_print(WARNING, "warning message");
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].level, WARNING);
    EXPECT_EQ(msgs[0].message, "warning message");
}

TEST(LogPrint, ErrorLevel) {
    LogSinkGuard guard;
    Log::log_print(ERR, "error message");
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].level, ERR);
    EXPECT_EQ(msgs[0].message, "error message");
}

// Note: FATAL calls exit(1), so we cannot test it in-process.

// ---------------------------------------------------------------------------
// Printf-style formatting
// ---------------------------------------------------------------------------

TEST(LogPrint, FormatsIntegerArguments) {
    LogSinkGuard guard;
    Log::log_print(INFO, "count=%d", 42);
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].message, "count=42");
}

TEST(LogPrint, FormatsStringArguments) {
    LogSinkGuard guard;
    Log::log_print(INFO, "name=%s age=%d", "Alice", 30);
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].message, "name=Alice age=30");
}

TEST(LogPrint, FormatsFloatArguments) {
    LogSinkGuard guard;
    Log::log_print(DEBUG, "value=%.2f", 3.14);
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].message, "value=3.14");
}

TEST(LogPrint, EmptyFormatString) {
    LogSinkGuard guard;
    Log::log_print(INFO, "");
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_EQ(msgs[0].message, "");
}

// ---------------------------------------------------------------------------
// Invalid log levels — no sink callback, no crash
// ---------------------------------------------------------------------------

TEST(LogPrint, InvalidLevelDoesNotFireSink) {
    LogSinkGuard guard;
    Log::log_print(INVALID, "should not appear");
    EXPECT_EQ(guard.count(), 0u);
}

TEST(LogPrint, CountLevelDoesNotFireSink) {
    LogSinkGuard guard;
    Log::log_print(COUNT, "should not appear");
    EXPECT_EQ(guard.count(), 0u);
}

TEST(LogPrint, OutOfRangeLevelDoesNotFireSink) {
    LogSinkGuard guard;
    Log::log_print(static_cast<LogLevel>(99), "should not appear");
    EXPECT_EQ(guard.count(), 0u);
}

// ---------------------------------------------------------------------------
// Timestamp presence
// ---------------------------------------------------------------------------

TEST(LogPrint, TimestampIsNonEmpty) {
    LogSinkGuard guard;
    Log::log_print(INFO, "timestamp check");
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    EXPECT_FALSE(msgs[0].timestamp.empty());
}

// ---------------------------------------------------------------------------
// Sink management
// ---------------------------------------------------------------------------

TEST(LogSink, SetSinkToNullptrClears) {
    // Install a sink, clear it, log a message — nothing captured.
    std::vector<CapturedMessage> captured;
    Log::set_sink([&](LogLevel level, const std::string& ts, const std::string& msg) {
        captured.push_back({level, ts, msg});
    });
    Log::set_sink(nullptr);

    Log::log_print(INFO, "after clear");
    EXPECT_TRUE(captured.empty());
}

TEST(LogSink, ReplacingSinkWorks) {
    // First sink captures to vec1, second to vec2.
    std::vector<CapturedMessage> vec1, vec2;

    Log::set_sink([&](LogLevel level, const std::string& ts, const std::string& msg) {
        vec1.push_back({level, ts, msg});
    });
    Log::log_print(INFO, "to first");

    Log::set_sink([&](LogLevel level, const std::string& ts, const std::string& msg) {
        vec2.push_back({level, ts, msg});
    });
    Log::log_print(INFO, "to second");

    Log::set_sink(nullptr);

    ASSERT_EQ(vec1.size(), 1u);
    EXPECT_EQ(vec1[0].message, "to first");

    ASSERT_EQ(vec2.size(), 1u);
    EXPECT_EQ(vec2[0].message, "to second");
}

// ---------------------------------------------------------------------------
// Multiple messages in sequence
// ---------------------------------------------------------------------------

TEST(LogPrint, MultipleMessagesPreserveOrder) {
    LogSinkGuard guard;
    Log::log_print(INFO, "first");
    Log::log_print(WARNING, "second");
    Log::log_print(ERR, "third");

    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 3u);
    EXPECT_EQ(msgs[0].message, "first");
    EXPECT_EQ(msgs[0].level, INFO);
    EXPECT_EQ(msgs[1].message, "second");
    EXPECT_EQ(msgs[1].level, WARNING);
    EXPECT_EQ(msgs[2].message, "third");
    EXPECT_EQ(msgs[2].level, ERR);
}

// ---------------------------------------------------------------------------
// Long message (ensure buffer doesn't overflow silently)
// ---------------------------------------------------------------------------

TEST(LogPrint, LongMessageIsTruncatedOrDelivered) {
    LogSinkGuard guard;
    // The internal buffer is 4096 bytes. A message longer than that
    // should be silently truncated by vsnprintf, not crash.
    std::string long_msg(8000, 'X');
    Log::log_print(INFO, "%s", long_msg.c_str());

    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 1u);
    // The message should be at most 4095 chars (null-terminated 4096 buffer).
    EXPECT_LE(msgs[0].message.size(), 4095u);
    EXPECT_GT(msgs[0].message.size(), 0u);
}

// ---------------------------------------------------------------------------
// All valid severity levels are delivered through the sink
// ---------------------------------------------------------------------------

TEST(LogPrint, AllValidLevelsAreDelivered) {
    LogSinkGuard guard;
    // FATAL is excluded because it calls exit(1).
    LogLevel levels[] = {VERBOSE, DEBUG, INFO, WARNING, ERR};
    for (auto lvl : levels) {
        Log::log_print(lvl, "level %d", static_cast<int>(lvl));
    }
    auto msgs = guard.drain();
    ASSERT_EQ(msgs.size(), 5u);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(msgs[i].level, levels[i]);
    }
}
