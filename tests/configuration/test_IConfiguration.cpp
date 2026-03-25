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

// Global callback receives the key that was set.
//   set("x", 10) → callback("x")
TEST_F(ConfigurationTest, CallbackFiredOnSetValue) {
    std::string notified_key;
    cfg().add_on_change([&](const std::string& key) { notified_key = key; });

    cfg().set_value("x", std::any{10});
    EXPECT_EQ(notified_key, "x");
}

// Global callback receives the key that was removed.
//   remove("y") → callback("y")
TEST_F(ConfigurationTest, CallbackFiredOnRemove) {
    cfg().set_value("y", std::any{1});

    std::string notified_key;
    cfg().add_on_change([&](const std::string& key) { notified_key = key; });

    cfg().remove("y");
    EXPECT_EQ(notified_key, "y");
}

// clear() is a bulk operation — callback receives an empty key.
//   clear() → callback("")
TEST_F(ConfigurationTest, CallbackFiredOnClear) {
    cfg().set_value("z", std::any{1});

    bool called = false;
    cfg().add_on_change([&](const std::string& key) {
        called = true;
        EXPECT_TRUE(key.empty());
    });

    cfg().clear();
    EXPECT_TRUE(called);
}

// deserialize() is a bulk operation — callback receives an empty key.
//   deserialize({...}) → callback("")
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

// Failed deserialize must not fire any callbacks.
//   deserialize({}) → (fails) → no callback
TEST_F(ConfigurationTest, CallbackNotFiredOnFailedDeserialize) {
    bool called = false;
    cfg().add_on_change([&](const std::string&) { called = true; });

    cfg().deserialize({}); // empty → fails
    EXPECT_FALSE(called);
}

// Mutating without any registered callbacks must not crash.
//   set/remove/clear with zero callbacks → no-op notification
TEST_F(ConfigurationTest, NoCallbackDoesNotCrash) {
    EXPECT_NO_THROW(cfg().set_value("a", std::any{1}));
    EXPECT_NO_THROW(cfg().remove("a"));
    EXPECT_NO_THROW(cfg().clear());
}

// Two independent global callbacks both fire for the same change.
//   set("x", 1) → callback_a("x"), callback_b("x")
TEST_F(ConfigurationTest, MultipleCallbacksFired) {
    int count_a = 0;
    int count_b = 0;
    cfg().add_on_change([&](const std::string&) { ++count_a; });
    cfg().add_on_change([&](const std::string&) { ++count_b; });

    cfg().set_value("x", std::any{1});
    EXPECT_EQ(count_a, 1);
    EXPECT_EQ(count_b, 1);
}

// A global callback can be unregistered by the ID returned from add_on_change.
//   add → set("x") → count=1 → remove_on_change(id) → set("y") → count=1
TEST_F(ConfigurationTest, RemoveCallbackById) {
    int count = 0;
    int id = cfg().add_on_change([&](const std::string&) { ++count; });

    cfg().set_value("x", std::any{1});
    EXPECT_EQ(count, 1);

    cfg().remove_on_change(id);
    cfg().set_value("y", std::any{2});
    EXPECT_EQ(count, 1);
}

// clear_on_change() removes every registered callback (global and key-filtered).
//   add(cb_a), add(cb_b) → clear_on_change() → set("x") → count=0
TEST_F(ConfigurationTest, ClearOnChangeRemovesAll) {
    int count = 0;
    cfg().add_on_change([&](const std::string&) { ++count; });
    cfg().add_on_change([&](const std::string&) { ++count; });

    cfg().clear_on_change();
    cfg().set_value("x", std::any{1});
    EXPECT_EQ(count, 0);
}

// ---------------------------------------------------------------------------
// Key-filtered callbacks
// ---------------------------------------------------------------------------

// Key-filtered callback fires only when the exact key is set.
//   filter="volume"  set("volume")→fires  set("brightness")→skipped
TEST_F(ConfigurationTest, KeyCallbackFiresOnMatchingKey) {
    int count = 0;
    cfg().add_on_change("volume", [&](const std::string& key) {
        EXPECT_EQ(key, "volume");
        ++count;
    });

    cfg().set_value("volume", std::any{80});
    cfg().set_value("brightness", std::any{50});
    cfg().set_value("volume", std::any{90});
    EXPECT_EQ(count, 2);
}

// Key-filtered callback stays silent when only unrelated keys change.
//   filter="music"  set("sfx")→skipped  set("voice")→skipped  remove("sfx")→skipped
TEST_F(ConfigurationTest, KeyCallbackIgnoresOtherKeys) {
    bool fired = false;
    cfg().add_on_change("music", [&](const std::string&) { fired = true; });

    cfg().set_value("sfx", std::any{100});
    cfg().set_value("voice", std::any{75});
    cfg().remove("sfx");
    EXPECT_FALSE(fired);
}

// Key-filtered callback fires when its watched key is removed.
//   filter="temp"  remove("temp") → callback("temp")
TEST_F(ConfigurationTest, KeyCallbackFiresOnRemoveMatchingKey) {
    cfg().set_value("temp", std::any{1});

    std::string notified;
    cfg().add_on_change("temp", [&](const std::string& key) { notified = key; });

    cfg().remove("temp");
    EXPECT_EQ(notified, "temp");
}

// Key-filtered callback fires on clear() with an empty key, so listeners
// know the entire store was wiped even if their specific key wasn't named.
//   filter="anything"  clear() → callback("")
TEST_F(ConfigurationTest, KeyCallbackFiresOnClearBulkOperation) {
    bool fired = false;
    cfg().add_on_change("anything", [&](const std::string& key) {
        EXPECT_TRUE(key.empty());
        fired = true;
    });

    cfg().clear();
    EXPECT_TRUE(fired);
}

// Key-filtered callback fires on deserialize() with an empty key, so
// listeners can re-read their value after a full reload.
//   filter="specific_key"  deserialize({...}) → callback("")
TEST_F(ConfigurationTest, KeyCallbackFiresOnDeserializeBulkOperation) {
    bool fired = false;
    cfg().add_on_change("specific_key", [&](const std::string& key) {
        EXPECT_TRUE(key.empty());
        fired = true;
    });

    std::vector<uint8_t> data{'h', 'i'};
    cfg().deserialize(data);
    EXPECT_TRUE(fired);
}

// Global and key-filtered callbacks can be registered side by side.
// The global fires for every change; the key-filtered fires only on match.
//   global + filter="target"
//   set("target")→both  set("other")→global only  set("target")→both
TEST_F(ConfigurationTest, GlobalAndKeyCallbacksCoexist) {
    int global_count = 0;
    int key_count = 0;

    cfg().add_on_change([&](const std::string&) { ++global_count; });
    cfg().add_on_change("target", [&](const std::string&) { ++key_count; });

    cfg().set_value("target", std::any{1});
    cfg().set_value("other", std::any{2});
    cfg().set_value("target", std::any{3});

    EXPECT_EQ(global_count, 3);
    EXPECT_EQ(key_count, 2);
}

// Two independent callbacks watching the same key both fire.
//   filter="shared" (x2)  set("shared") → cb_a + cb_b
TEST_F(ConfigurationTest, MultipleKeyCallbacksSameKey) {
    int count_a = 0;
    int count_b = 0;

    cfg().add_on_change("shared", [&](const std::string&) { ++count_a; });
    cfg().add_on_change("shared", [&](const std::string&) { ++count_b; });

    cfg().set_value("shared", std::any{42});
    EXPECT_EQ(count_a, 1);
    EXPECT_EQ(count_b, 1);
}

// Callbacks on different keys fire independently and never cross-trigger.
//   filter="alpha" + filter="beta"
//   set("alpha")→alpha_cb  set("beta")→beta_cb  set("gamma")→neither
TEST_F(ConfigurationTest, MultipleKeyCallbacksDifferentKeys) {
    int alpha_count = 0;
    int beta_count = 0;

    cfg().add_on_change("alpha", [&](const std::string&) { ++alpha_count; });
    cfg().add_on_change("beta", [&](const std::string&) { ++beta_count; });

    cfg().set_value("alpha", std::any{1});
    cfg().set_value("beta", std::any{2});
    cfg().set_value("alpha", std::any{3});
    cfg().set_value("gamma", std::any{4});

    EXPECT_EQ(alpha_count, 2);
    EXPECT_EQ(beta_count, 1);
}

// A key-filtered callback can be unregistered by its ID, just like global ones.
//   add("target", cb) → set → count=1 → remove_on_change(id) → set → count=1
TEST_F(ConfigurationTest, RemoveKeyCallbackById) {
    int count = 0;
    int id = cfg().add_on_change("target", [&](const std::string&) { ++count; });

    cfg().set_value("target", std::any{1});
    EXPECT_EQ(count, 1);

    cfg().remove_on_change(id);
    cfg().set_value("target", std::any{2});
    EXPECT_EQ(count, 1);
}

// clear_on_change() wipes both global and key-filtered callbacks.
//   global_cb + filter="x" → clear_on_change() → set("x") → nothing fires
TEST_F(ConfigurationTest, ClearOnChangeRemovesKeyCallbacksToo) {
    int global_count = 0;
    int key_count = 0;

    cfg().add_on_change([&](const std::string&) { ++global_count; });
    cfg().add_on_change("x", [&](const std::string&) { ++key_count; });

    cfg().clear_on_change();
    cfg().set_value("x", std::any{1});
    EXPECT_EQ(global_count, 0);
    EXPECT_EQ(key_count, 0);
}

// Key filter uses exact string match — substrings and superstrings are ignored.
//   filter="volume"  set("volume_max")→skip  set("master_volume")→skip  set("vol")→skip
TEST_F(ConfigurationTest, KeyCallbackExactMatchOnly) {
    bool fired = false;
    cfg().add_on_change("volume", [&](const std::string&) { fired = true; });

    cfg().set_value("volume_max", std::any{100});
    cfg().set_value("master_volume", std::any{80});
    cfg().set_value("vol", std::any{50});
    EXPECT_FALSE(fired);
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
