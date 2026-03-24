#include "configuration/IConfiguration.h"

#include <gtest/gtest.h>

#include <algorithm>
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

    std::vector<std::string> do_keys() const override {
        std::vector<std::string> result;
        for (const auto& [k, v] : store_)
            result.push_back(k);
        return result;
    }

    void do_for_each(const KeyValueVisitor& visitor) const override {
        for (const auto& [k, v] : store_)
            visitor(k, v);
    }

  private:
    std::map<std::string, std::any> store_;
};

// Helper: get a fresh, empty TestConfig instance for each test.
// The singleton is static, so we clear it before every test.
class ConfigurationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        cfg().clear_on_change();
        cfg().clear();
    }

    static TestConfig& cfg() {
        return TestConfig::instance();
    }
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
// keys / for_each
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, KeysEmptyOnFreshConfig) {
    EXPECT_TRUE(cfg().keys().empty());
}

TEST_F(ConfigurationTest, KeysReturnsAllSetKeys) {
    cfg().set_value("a", std::any{1});
    cfg().set_value("b", std::any{2});
    cfg().set_value("c", std::any{3});
    auto k = cfg().keys();
    std::sort(k.begin(), k.end());
    ASSERT_EQ(k.size(), 3u);
    EXPECT_EQ(k[0], "a");
    EXPECT_EQ(k[1], "b");
    EXPECT_EQ(k[2], "c");
}

TEST_F(ConfigurationTest, ForEachVisitsAllPairs) {
    cfg().set_value("x", std::any{10});
    cfg().set_value("y", std::any{20});

    std::map<std::string, int> visited;
    cfg().for_each([&](const std::string& key, const std::any& val) {
        visited[key] = std::any_cast<int>(val);
    });

    EXPECT_EQ(visited.size(), 2u);
    EXPECT_EQ(visited["x"], 10);
    EXPECT_EQ(visited["y"], 20);
}

TEST_F(ConfigurationTest, ForEachEmptyConfig) {
    int count = 0;
    cfg().for_each([&](const std::string&, const std::any&) { ++count; });
    EXPECT_EQ(count, 0);
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
    cfg().add_on_change([&](const std::string& key) { notified_key = key; });

    cfg().set_value("x", std::any{10});
    EXPECT_EQ(notified_key, "x");
}

TEST_F(ConfigurationTest, CallbackFiredOnRemove) {
    cfg().set_value("y", std::any{1});

    std::string notified_key;
    cfg().add_on_change([&](const std::string& key) { notified_key = key; });

    cfg().remove("y");
    EXPECT_EQ(notified_key, "y");
}

TEST_F(ConfigurationTest, CallbackFiredOnClear) {
    cfg().set_value("z", std::any{1});

    bool called = false;
    cfg().add_on_change([&](const std::string& key) {
        called = true;
        EXPECT_TRUE(key.empty()); // clear() notifies with empty key
    });

    cfg().clear();
    EXPECT_TRUE(called);
}

TEST_F(ConfigurationTest, CallbackFiredOnDeserialize) {
    bool called = false;
    cfg().add_on_change([&](const std::string& key) {
        called = true;
        EXPECT_TRUE(key.empty());
    });

    std::vector<uint8_t> data{'h', 'i'};
    cfg().deserialize(data);
    EXPECT_TRUE(called);
}

TEST_F(ConfigurationTest, CallbackNotFiredOnFailedDeserialize) {
    bool called = false;
    cfg().add_on_change([&](const std::string&) { called = true; });

    cfg().deserialize({}); // empty → fails
    EXPECT_FALSE(called);
}

TEST_F(ConfigurationTest, NoCallbackDoesNotCrash) {
    // No callback set — mutating operations should not crash.
    EXPECT_NO_THROW(cfg().set_value("a", std::any{1}));
    EXPECT_NO_THROW(cfg().remove("a"));
    EXPECT_NO_THROW(cfg().clear());
}

TEST_F(ConfigurationTest, MultipleCallbacksFired) {
    int count_a = 0;
    int count_b = 0;
    cfg().add_on_change([&](const std::string&) { ++count_a; });
    cfg().add_on_change([&](const std::string&) { ++count_b; });

    cfg().set_value("x", std::any{1});
    EXPECT_EQ(count_a, 1);
    EXPECT_EQ(count_b, 1);
}

TEST_F(ConfigurationTest, RemoveCallbackById) {
    int count = 0;
    int id = cfg().add_on_change([&](const std::string&) { ++count; });

    cfg().set_value("x", std::any{1});
    EXPECT_EQ(count, 1);

    cfg().remove_on_change(id);
    cfg().set_value("y", std::any{2});
    EXPECT_EQ(count, 1); // callback was removed, count unchanged
}

TEST_F(ConfigurationTest, ClearOnChangeRemovesAll) {
    int count = 0;
    cfg().add_on_change([&](const std::string&) { ++count; });
    cfg().add_on_change([&](const std::string&) { ++count; });

    cfg().clear_on_change();
    cfg().set_value("x", std::any{1});
    EXPECT_EQ(count, 0);
}

// ---------------------------------------------------------------------------
// Basic thread safety (smoke test)
// ---------------------------------------------------------------------------

TEST_F(ConfigurationTest, ConcurrentSetAndGet) {
    constexpr int iterations = 1000;
    std::atomic<bool> go{false};

    auto writer = [&] {
        while (!go.load()) {
        }
        for (int i = 0; i < iterations; ++i)
            cfg().set_value("counter", std::any{i});
    };

    auto reader = [&] {
        while (!go.load()) {
        }
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
