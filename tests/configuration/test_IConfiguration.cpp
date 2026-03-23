#include "configuration/IConfiguration.h"

#include <gtest/gtest.h>

#include <any>
#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Minimal concrete subclass used by every test.
// ---------------------------------------------------------------------------

class TestConfig : public ConfigurationBase<TestConfig> {
  protected:
    bool do_deserialize(const std::vector<uint8_t>& data) override {
        store_.clear();
        // Trivial format: treat entire blob as a single string value under "blob".
        if (data.empty())
            return false;
        store_["blob"] = std::string(data.begin(), data.end());
        return true;
    }

    std::vector<uint8_t> do_serialize() const override {
        std::vector<uint8_t> out;
        for (const auto& [k, v] : store_) {
            if (v.type() == typeid(std::string)) {
                auto s = std::any_cast<std::string>(v);
                out.insert(out.end(), s.begin(), s.end());
            }
        }
        return out;
    }

    void do_set_value(const std::string& key, const std::any& value) override {
        store_[key] = value;
    }

    std::any do_value(const std::string& key, const std::any& default_value) const override {
        auto it = store_.find(key);
        return it != store_.end() ? it->second : default_value;
    }

    bool do_contains(const std::string& key) const override {
        return store_.count(key) > 0;
    }

    void do_remove(const std::string& key) override {
        store_.erase(key);
    }

    void do_clear() override {
        store_.clear();
    }

  private:
    std::map<std::string, std::any> store_;
};

// Helper: get a fresh, empty TestConfig instance for each test.
// The singleton is static, so we clear it before every test.
class ConfigurationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        cfg().set_on_change(nullptr);
        cfg().clear();
    }

    static TestConfig& cfg() { return TestConfig::instance(); }
};

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, InstanceReturnsSameObject) {
    EXPECT_EQ(&TestConfig::instance(), &TestConfig::instance());
}

// ---------------------------------------------------------------------------
// set_value / value round-trip
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, SetAndGetValue) {
    cfg().set_value("key", std::any{42});
    auto result = cfg().value("key");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::any_cast<int>(result), 42);
}

TEST_F(ConfigurationTest, ValueReturnsDefaultWhenMissing) {
    auto result = cfg().value("missing", std::any{std::string("fallback")});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::any_cast<std::string>(result), "fallback");
}

TEST_F(ConfigurationTest, ValueReturnsEmptyAnyWhenMissingNoDefault) {
    auto result = cfg().value("missing");
    EXPECT_FALSE(result.has_value());
}

// ---------------------------------------------------------------------------
// Typed value<T> template
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, TypedValueReturnsCorrectType) {
    cfg().set_value("str", std::any{std::string("hello")});
    auto result = cfg().value<std::string>("str");
    EXPECT_EQ(result, "hello");
}

TEST_F(ConfigurationTest, TypedValueReturnsDefaultOnMissing) {
    EXPECT_EQ(cfg().value<int>("no_such_key", 99), 99);
}

TEST_F(ConfigurationTest, TypedValueReturnsDefaultOnTypeMismatch) {
    cfg().set_value("int_key", std::any{42});
    // Ask for a string — type mismatch should give us the default.
    auto result = cfg().value<std::string>("int_key", "default");
    EXPECT_EQ(result, "default");
}

TEST_F(ConfigurationTest, TypedValueDefaultConstructedDefault) {
    // T{} default: int → 0
    EXPECT_EQ(cfg().value<int>("absent"), 0);
}

// ---------------------------------------------------------------------------
// contains
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, ContainsReturnsFalseForMissing) {
    EXPECT_FALSE(cfg().contains("nope"));
}

TEST_F(ConfigurationTest, ContainsReturnsTrueAfterSet) {
    cfg().set_value("present", std::any{1});
    EXPECT_TRUE(cfg().contains("present"));
}

// ---------------------------------------------------------------------------
// remove
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, RemoveDeletesKey) {
    cfg().set_value("tmp", std::any{1});
    ASSERT_TRUE(cfg().contains("tmp"));
    cfg().remove("tmp");
    EXPECT_FALSE(cfg().contains("tmp"));
}

TEST_F(ConfigurationTest, RemoveNonexistentKeyIsSafe) {
    EXPECT_NO_THROW(cfg().remove("nonexistent"));
}

// ---------------------------------------------------------------------------
// clear
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, ClearRemovesAllKeys) {
    cfg().set_value("a", std::any{1});
    cfg().set_value("b", std::any{2});
    cfg().clear();
    EXPECT_FALSE(cfg().contains("a"));
    EXPECT_FALSE(cfg().contains("b"));
}

// ---------------------------------------------------------------------------
// serialize / deserialize
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, SerializeRoundTrip) {
    cfg().set_value("blob", std::any{std::string("payload")});
    auto bytes = cfg().serialize();
    EXPECT_FALSE(bytes.empty());

    cfg().clear();
    EXPECT_TRUE(cfg().deserialize(bytes));
    EXPECT_EQ(cfg().value<std::string>("blob"), "payload");
}

TEST_F(ConfigurationTest, DeserializeEmptyDataReturnsFalse) {
    EXPECT_FALSE(cfg().deserialize({}));
}

// ---------------------------------------------------------------------------
// Change callback
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, CallbackFiredOnSetValue) {
    std::string notified_key;
    cfg().set_on_change([&](const std::string& key) { notified_key = key; });

    cfg().set_value("x", std::any{10});
    EXPECT_EQ(notified_key, "x");
}

TEST_F(ConfigurationTest, CallbackFiredOnRemove) {
    cfg().set_value("y", std::any{1});

    std::string notified_key;
    cfg().set_on_change([&](const std::string& key) { notified_key = key; });

    cfg().remove("y");
    EXPECT_EQ(notified_key, "y");
}

TEST_F(ConfigurationTest, CallbackFiredOnClear) {
    cfg().set_value("z", std::any{1});

    bool called = false;
    cfg().set_on_change([&](const std::string& key) {
        called = true;
        EXPECT_TRUE(key.empty()); // clear() notifies with empty key
    });

    cfg().clear();
    EXPECT_TRUE(called);
}

TEST_F(ConfigurationTest, CallbackFiredOnDeserialize) {
    bool called = false;
    cfg().set_on_change([&](const std::string& key) {
        called = true;
        EXPECT_TRUE(key.empty());
    });

    std::vector<uint8_t> data{'h', 'i'};
    cfg().deserialize(data);
    EXPECT_TRUE(called);
}

TEST_F(ConfigurationTest, CallbackNotFiredOnFailedDeserialize) {
    bool called = false;
    cfg().set_on_change([&](const std::string&) { called = true; });

    cfg().deserialize({}); // empty → fails
    EXPECT_FALSE(called);
}

TEST_F(ConfigurationTest, NoCallbackDoesNotCrash) {
    // No callback set — mutating operations should not crash.
    EXPECT_NO_THROW(cfg().set_value("a", std::any{1}));
    EXPECT_NO_THROW(cfg().remove("a"));
    EXPECT_NO_THROW(cfg().clear());
}

// ---------------------------------------------------------------------------
// Basic thread safety (smoke test)
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, ConcurrentSetAndGet) {
    constexpr int iterations = 1000;
    std::atomic<bool> go{false};

    auto writer = [&] {
        while (!go.load()) {}
        for (int i = 0; i < iterations; ++i)
            cfg().set_value("counter", std::any{i});
    };

    auto reader = [&] {
        while (!go.load()) {}
        for (int i = 0; i < iterations; ++i) {
            auto v = cfg().value("counter");
            // Value may or may not be set yet; just ensure no crash.
            (void)v;
        }
    };

    std::thread t1(writer);
    std::thread t2(reader);
    go.store(true);
    t1.join();
    t2.join();

    // After writer finishes, final value should be the last written.
    EXPECT_EQ(cfg().value<int>("counter"), iterations - 1);
}
