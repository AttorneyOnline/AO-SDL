#include <gtest/gtest.h>

#include "configuration/JsonConfiguration.h"

#include <algorithm>
#include <any>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

class MockJsonConfiguration : public JsonConfiguration<MockJsonConfiguration> {};

// ---------------------------------------------------------------------------
// Fixture — resets the UserConfiguration singleton between tests.
// ---------------------------------------------------------------------------

class JsonConfigurationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        cfg().clear_on_change();
        cfg().clear();
    }

    static MockJsonConfiguration& cfg() {
        return MockJsonConfiguration::instance();
    }
};

// ---------------------------------------------------------------------------
// Basic typed round-trips
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, StringRoundTrip) {
    cfg().set_value("name", std::any{std::string("Phoenix")});
    EXPECT_EQ(cfg().value<std::string>("name"), "Phoenix");
}

TEST_F(JsonConfigurationTest, IntRoundTrip) {
    cfg().set_value("volume", std::any{75});
    EXPECT_EQ(cfg().value<int>("volume"), 75);
}

TEST_F(JsonConfigurationTest, BoolRoundTrip) {
    cfg().set_value("fullscreen", std::any{true});
    EXPECT_TRUE(cfg().value<bool>("fullscreen"));
}

TEST_F(JsonConfigurationTest, DoubleRoundTrip) {
    cfg().set_value("scale", std::any{1.5});
    EXPECT_DOUBLE_EQ(cfg().value<double>("scale"), 1.5);
}

TEST_F(JsonConfigurationTest, Int64RoundTrip) {
    int64_t big = 1LL << 40;
    cfg().set_value("big", std::any{big});
    EXPECT_EQ(cfg().value<int64_t>("big"), big);
}

TEST_F(JsonConfigurationTest, UnsignedIntRoundTrip) {
    cfg().set_value("uint_val", std::any{42u});
    EXPECT_EQ(cfg().value<unsigned int>("uint_val"), 42u);
}

TEST_F(JsonConfigurationTest, FloatRoundTrip) {
    cfg().set_value("fval", std::any{1.5f});
    EXPECT_FLOAT_EQ(cfg().value<float>("fval"), 1.5f);
}

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, MissingKeyReturnsTypedDefault) {
    EXPECT_EQ(cfg().value<int>("missing", 42), 42);
}

TEST_F(JsonConfigurationTest, MissingKeyReturnsDefaultConstructed) {
    EXPECT_EQ(cfg().value<int>("missing"), 0);
}

// ---------------------------------------------------------------------------
// JSON serialization round-trip
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, SerializeDeserializeRoundTrip) {
    cfg().set_value("username", std::any{std::string("Edgeworth")});
    cfg().set_value("port", std::any{27016});
    cfg().set_value("widescreen", std::any{false});

    auto bytes = cfg().serialize();
    ASSERT_FALSE(bytes.empty());

    // Verify the serialized form is valid JSON.
    std::string json_str(bytes.begin(), bytes.end());
    EXPECT_NO_THROW(auto parsed = nlohmann::json::parse(json_str));

    // Clear and restore.
    cfg().clear();
    EXPECT_FALSE(cfg().contains("username"));

    EXPECT_TRUE(cfg().deserialize(bytes));
    EXPECT_EQ(cfg().value<std::string>("username"), "Edgeworth");
    EXPECT_EQ(cfg().value<int>("port"), 27016);
    EXPECT_FALSE(cfg().value<bool>("widescreen"));
}

// ---------------------------------------------------------------------------
// Deserialize from a known JSON blob
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, DeserializeFromSampleJson) {
    const std::string sample = R"({
        "username": "Judge",
        "server":   "localhost",
        "port":     27016,
        "theme":    "default",
        "volume":   80,
        "effects":  true,
        "scale":    2.0
    })";
    std::vector<uint8_t> data(sample.begin(), sample.end());
    ASSERT_TRUE(cfg().deserialize(data));

    EXPECT_EQ(cfg().value<std::string>("username"), "Judge");
    EXPECT_EQ(cfg().value<std::string>("server"), "localhost");
    EXPECT_EQ(cfg().value<int>("port"), 27016);
    EXPECT_EQ(cfg().value<std::string>("theme"), "default");
    EXPECT_EQ(cfg().value<int>("volume"), 80);
    EXPECT_TRUE(cfg().value<bool>("effects"));
    EXPECT_DOUBLE_EQ(cfg().value<double>("scale"), 2.0);
}

// ---------------------------------------------------------------------------
// Deserialize invalid data
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, DeserializeInvalidJsonReturnsFalse) {
    std::string bad = "not json at all {{{";
    std::vector<uint8_t> data(bad.begin(), bad.end());
    EXPECT_FALSE(cfg().deserialize(data));
}

TEST_F(JsonConfigurationTest, DeserializeArrayReturnsFalse) {
    std::string arr = "[1, 2, 3]";
    std::vector<uint8_t> data(arr.begin(), arr.end());
    EXPECT_FALSE(cfg().deserialize(data));
}

TEST_F(JsonConfigurationTest, DeserializeEmptyReturnsFalse) {
    EXPECT_FALSE(cfg().deserialize({}));
}

// ---------------------------------------------------------------------------
// Overwrite, remove, clear
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, OverwriteExistingKey) {
    cfg().set_value("k", std::any{1});
    cfg().set_value("k", std::any{2});
    EXPECT_EQ(cfg().value<int>("k"), 2);
}

TEST_F(JsonConfigurationTest, RemoveKey) {
    cfg().set_value("tmp", std::any{std::string("val")});
    ASSERT_TRUE(cfg().contains("tmp"));
    cfg().remove("tmp");
    EXPECT_FALSE(cfg().contains("tmp"));
}

TEST_F(JsonConfigurationTest, ClearRemovesAll) {
    cfg().set_value("a", std::any{1});
    cfg().set_value("b", std::any{2});
    cfg().clear();
    EXPECT_FALSE(cfg().contains("a"));
    EXPECT_FALSE(cfg().contains("b"));

    // Serialized form should be an empty JSON object.
    auto bytes = cfg().serialize();
    std::string s(bytes.begin(), bytes.end());
    EXPECT_EQ(s, "{}");
}

// ---------------------------------------------------------------------------
// const char* convenience
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, ConstCharStarStoredAsString) {
    cfg().set_value("greeting", std::any{static_cast<const char*>("hello")});
    EXPECT_EQ(cfg().value<std::string>("greeting"), "hello");
}

// ---------------------------------------------------------------------------
// Change callback fires on deserialize
// ---------------------------------------------------------------------------

// Global callback fires when JSON is deserialized (bulk operation, empty key).
//   deserialize({"k":1}) → callback("")
TEST_F(JsonConfigurationTest, CallbackOnDeserialize) {
    bool fired = false;
    cfg().add_on_change([&](const std::string&) { fired = true; });

    const std::string json = R"({"k": 1})";
    std::vector<uint8_t> data(json.begin(), json.end());
    cfg().deserialize(data);
    EXPECT_TRUE(fired);
}

// ---------------------------------------------------------------------------
// Path-based access  ("key/index" and "key/subkey")
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, PathAccessArrayElement) {
    const std::string json = R"({"servers": ["alpha", "bravo", "charlie"]})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    EXPECT_EQ(cfg().value<std::string>("servers/0"), "alpha");
    EXPECT_EQ(cfg().value<std::string>("servers/1"), "bravo");
    EXPECT_EQ(cfg().value<std::string>("servers/2"), "charlie");
}

TEST_F(JsonConfigurationTest, PathAccessNestedObject) {
    const std::string json = R"({"display": {"width": 1920, "height": 1080}})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    EXPECT_EQ(cfg().value<int>("display/width"), 1920);
    EXPECT_EQ(cfg().value<int>("display/height"), 1080);
}

TEST_F(JsonConfigurationTest, PathAccessDeepNesting) {
    const std::string json = R"({"a": {"b": {"c": 42}}})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    EXPECT_EQ(cfg().value<int>("a/b/c"), 42);
}

TEST_F(JsonConfigurationTest, PathAccessArrayOfObjects) {
    const std::string json = R"({
        "servers": [
            {"name": "alpha", "port": 27016},
            {"name": "bravo", "port": 27017}
        ]
    })";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    EXPECT_EQ(cfg().value<std::string>("servers/0/name"), "alpha");
    EXPECT_EQ(cfg().value<int>("servers/0/port"), 27016);
    EXPECT_EQ(cfg().value<std::string>("servers/1/name"), "bravo");
    EXPECT_EQ(cfg().value<int>("servers/1/port"), 27017);
}

TEST_F(JsonConfigurationTest, PathContains) {
    const std::string json = R"({"items": [10, 20, 30]})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    EXPECT_TRUE(cfg().contains("items/0"));
    EXPECT_TRUE(cfg().contains("items/2"));
    EXPECT_FALSE(cfg().contains("items/3"));
    EXPECT_FALSE(cfg().contains("items/999"));
}

TEST_F(JsonConfigurationTest, PathSetValueCreatesStructure) {
    cfg().set_value("audio/master_volume", std::any{80});
    cfg().set_value("audio/music_volume", std::any{60});

    EXPECT_EQ(cfg().value<int>("audio/master_volume"), 80);
    EXPECT_EQ(cfg().value<int>("audio/music_volume"), 60);
    EXPECT_TRUE(cfg().contains("audio"));
}

TEST_F(JsonConfigurationTest, PathRemoveArrayElement) {
    const std::string json = R"({"colors": ["red", "green", "blue"]})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    cfg().remove("colors/1"); // remove "green"
    EXPECT_EQ(cfg().value<std::string>("colors/0"), "red");
    EXPECT_EQ(cfg().value<std::string>("colors/1"), "blue"); // shifted
    EXPECT_FALSE(cfg().contains("colors/2"));
}

TEST_F(JsonConfigurationTest, PathRemoveNestedKey) {
    const std::string json = R"({"display": {"width": 1920, "height": 1080}})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    cfg().remove("display/height");
    EXPECT_TRUE(cfg().contains("display/width"));
    EXPECT_FALSE(cfg().contains("display/height"));
}

TEST_F(JsonConfigurationTest, PathMissingReturnsDefault) {
    EXPECT_EQ(cfg().value<int>("nonexistent/0", 99), 99);
    EXPECT_EQ(cfg().value<std::string>("a/b/c", "fallback"), "fallback");
}

TEST_F(JsonConfigurationTest, PathRoundTripThroughSerialize) {
    cfg().set_value("net/server", std::any{std::string("localhost")});
    cfg().set_value("net/port", std::any{27016});

    auto bytes = cfg().serialize();
    cfg().clear();
    ASSERT_TRUE(cfg().deserialize(bytes));

    EXPECT_EQ(cfg().value<std::string>("net/server"), "localhost");
    EXPECT_EQ(cfg().value<int>("net/port"), 27016);
}

// ---------------------------------------------------------------------------
// keys / for_each — flat
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, KeysFlatValues) {
    cfg().set_value("a", std::any{1});
    cfg().set_value("b", std::any{std::string("hello")});
    auto k = cfg().keys();
    std::sort(k.begin(), k.end());
    ASSERT_EQ(k.size(), 2u);
    EXPECT_EQ(k[0], "a");
    EXPECT_EQ(k[1], "b");
}

TEST_F(JsonConfigurationTest, KeysEmpty) {
    EXPECT_TRUE(cfg().keys().empty());
}

TEST_F(JsonConfigurationTest, ForEachFlatValues) {
    cfg().set_value("x", std::any{42});
    cfg().set_value("y", std::any{std::string("hi")});

    std::map<std::string, std::any> visited;
    cfg().for_each([&](const std::string& key, const std::any& val) { visited[key] = val; });

    ASSERT_EQ(visited.size(), 2u);
    EXPECT_EQ(std::any_cast<int64_t>(visited["x"]), 42); // natural mapping: int → int64_t
    EXPECT_EQ(std::any_cast<std::string>(visited["y"]), "hi");
}

// ---------------------------------------------------------------------------
// keys / for_each — nested objects and arrays
// ---------------------------------------------------------------------------

TEST_F(JsonConfigurationTest, KeysNestedObject) {
    const std::string json = R"({"display": {"width": 1920, "height": 1080}})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    auto k = cfg().keys();
    std::sort(k.begin(), k.end());
    ASSERT_EQ(k.size(), 2u);
    EXPECT_EQ(k[0], "display/height");
    EXPECT_EQ(k[1], "display/width");
}

TEST_F(JsonConfigurationTest, KeysArrayElements) {
    const std::string json = R"({"colors": ["red", "green", "blue"]})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    auto k = cfg().keys();
    std::sort(k.begin(), k.end());
    ASSERT_EQ(k.size(), 3u);
    EXPECT_EQ(k[0], "colors/0");
    EXPECT_EQ(k[1], "colors/1");
    EXPECT_EQ(k[2], "colors/2");
}

TEST_F(JsonConfigurationTest, KeysDeepNesting) {
    const std::string json = R"({"a": {"b": {"c": 1, "d": 2}}})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    auto k = cfg().keys();
    std::sort(k.begin(), k.end());
    ASSERT_EQ(k.size(), 2u);
    EXPECT_EQ(k[0], "a/b/c");
    EXPECT_EQ(k[1], "a/b/d");
}

TEST_F(JsonConfigurationTest, KeysMixedFlatAndNested) {
    const std::string json = R"({"name": "test", "audio": {"volume": 80}})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    auto k = cfg().keys();
    std::sort(k.begin(), k.end());
    ASSERT_EQ(k.size(), 2u);
    EXPECT_EQ(k[0], "audio/volume");
    EXPECT_EQ(k[1], "name");
}

TEST_F(JsonConfigurationTest, ForEachNestedValues) {
    const std::string json = R"({"servers": [{"name": "alpha"}, {"name": "bravo"}]})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    std::map<std::string, std::string> visited;
    cfg().for_each(
        [&](const std::string& key, const std::any& val) { visited[key] = std::any_cast<std::string>(val); });

    ASSERT_EQ(visited.size(), 2u);
    EXPECT_EQ(visited["servers/0/name"], "alpha");
    EXPECT_EQ(visited["servers/1/name"], "bravo");
}

// ---------------------------------------------------------------------------
// Key-filtered callbacks with path-based keys
// ---------------------------------------------------------------------------

// Path-filtered callback fires only when the exact nested path is set.
//   filter="audio/master_volume"
//   set("audio/master_volume")→fires  set("audio/music_volume")→skipped
TEST_F(JsonConfigurationTest, PathKeyCallbackFiresOnNestedSet) {
    int count = 0;
    cfg().add_on_change("audio/master_volume", [&](const std::string& key) {
        EXPECT_EQ(key, "audio/master_volume");
        ++count;
    });

    cfg().set_value("audio/master_volume", std::any{80});
    cfg().set_value("audio/music_volume", std::any{60});
    cfg().set_value("audio/master_volume", std::any{90});
    EXPECT_EQ(count, 2);
}

// Path-filtered callback ignores sibling keys under the same parent.
//   filter="display/width"  set("display/height")→skip  set("display/fullscreen")→skip
TEST_F(JsonConfigurationTest, PathKeyCallbackIgnoresSiblingPaths) {
    bool fired = false;
    cfg().add_on_change("display/width", [&](const std::string&) { fired = true; });

    cfg().set_value("display/height", std::any{1080});
    cfg().set_value("display/fullscreen", std::any{true});
    EXPECT_FALSE(fired);
}

// Path-filtered callback fires when the watched nested key is removed.
//   filter="net/host"  remove("net/host") → callback("net/host")
TEST_F(JsonConfigurationTest, PathKeyCallbackFiresOnRemove) {
    const std::string json = R"({"net": {"host": "localhost", "port": 27016}})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    std::string notified;
    cfg().add_on_change("net/host", [&](const std::string& key) { notified = key; });

    cfg().remove("net/host");
    EXPECT_EQ(notified, "net/host");
}

// Path-filtered callback works at arbitrary nesting depth (4 levels deep).
//   filter="a/b/c/d"
//   set("a/b/c/d")→fires  set("a/b/c/e")→skip  set("a/b/x")→skip
TEST_F(JsonConfigurationTest, PathKeyCallbackDeepNestedKey) {
    int count = 0;
    cfg().add_on_change("a/b/c/d", [&](const std::string& key) {
        EXPECT_EQ(key, "a/b/c/d");
        ++count;
    });

    cfg().set_value("a/b/c/d", std::any{42});
    cfg().set_value("a/b/c/e", std::any{99});
    cfg().set_value("a/b/x", std::any{0});
    cfg().set_value("a/b/c/d", std::any{43});
    EXPECT_EQ(count, 2);
}

// Path-filtered callback with array index fires only for the exact index+field.
//   filter="servers/1/port"
//   set("servers/0/port")→skip  set("servers/1/name")→skip
//   set("servers/1/port")→fires  set("servers/2/port")→skip
TEST_F(JsonConfigurationTest, ArrayPathCallbackFiresForSpecificIndex) {
    const std::string json = R"({"servers": [
        {"name": "alpha", "port": 27016},
        {"name": "bravo", "port": 27017},
        {"name": "charlie", "port": 27018}
    ]})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    int count = 0;
    cfg().add_on_change("servers/1/port", [&](const std::string& key) {
        EXPECT_EQ(key, "servers/1/port");
        ++count;
    });

    cfg().set_value("servers/0/port", std::any{9000});
    cfg().set_value("servers/1/name", std::any{std::string("delta")});
    cfg().set_value("servers/1/port", std::any{9001});
    cfg().set_value("servers/2/port", std::any{9002});
    EXPECT_EQ(count, 1);
}

// Global and path-filtered callbacks registered together fire independently.
//   global + filter="video/resolution/width"
//   set("video/resolution/width")→both  set("video/resolution/height")→global only
TEST_F(JsonConfigurationTest, GlobalAndPathCallbacksCoexist) {
    int global_count = 0;
    int path_count = 0;

    cfg().add_on_change([&](const std::string&) { ++global_count; });
    cfg().add_on_change("video/resolution/width", [&](const std::string&) { ++path_count; });

    cfg().set_value("video/resolution/width", std::any{1920});
    cfg().set_value("video/resolution/height", std::any{1080});
    cfg().set_value("video/vsync", std::any{true});
    cfg().set_value("video/resolution/width", std::any{2560});

    EXPECT_EQ(global_count, 4);
    EXPECT_EQ(path_count, 2);
}

// Path-filtered callbacks receive empty-key notifications from bulk operations
// (clear/deserialize) so listeners can re-read their value after a full reload.
//   filter="audio/volume"  clear() → callback("")
//   filter="net/port"      deserialize({...}) → callback("")
TEST_F(JsonConfigurationTest, PathCallbackFiresOnBulkClearAndDeserialize) {
    int clear_count = 0;
    int deser_count = 0;

    cfg().add_on_change("audio/volume", [&](const std::string& key) {
        if (key.empty())
            ++clear_count;
    });

    cfg().clear();
    EXPECT_EQ(clear_count, 1);

    cfg().add_on_change("net/port", [&](const std::string& key) {
        if (key.empty())
            ++deser_count;
    });

    const std::string json = R"({"net": {"port": 27016}})";
    std::vector<uint8_t> data(json.begin(), json.end());
    cfg().deserialize(data);
    EXPECT_EQ(deser_count, 1);
}

// Callbacks at different nesting depths fire independently.
//   filter="ui/theme" (depth 2) + filter="ui/panels/left/width" (depth 4)
//   set("ui/theme")→shallow  set("ui/panels/left/width")→deep
//   set("ui/panels/left/visible")→neither  set("ui/panels/right/width")→neither
TEST_F(JsonConfigurationTest, MultiplePathCallbacksDifferentDepths) {
    int shallow_count = 0;
    int deep_count = 0;

    cfg().add_on_change("ui/theme", [&](const std::string&) { ++shallow_count; });
    cfg().add_on_change("ui/panels/left/width", [&](const std::string&) { ++deep_count; });

    cfg().set_value("ui/theme", std::any{std::string("dark")});
    cfg().set_value("ui/panels/left/width", std::any{300});
    cfg().set_value("ui/panels/left/visible", std::any{true});
    cfg().set_value("ui/theme", std::any{std::string("light")});
    cfg().set_value("ui/panels/right/width", std::any{250});

    EXPECT_EQ(shallow_count, 2);
    EXPECT_EQ(deep_count, 1);
}

// A parent path filter must not match children — exact path match only.
//   filter="servers/0"  set("servers/0/name")→skip  set("servers/0/port")→skip
TEST_F(JsonConfigurationTest, PathCallbackExactMatchNoPartialPrefix) {
    bool fired = false;
    cfg().add_on_change("servers/0", [&](const std::string&) { fired = true; });

    cfg().set_value("servers/0/name", std::any{std::string("alpha")});
    cfg().set_value("servers/0/port", std::any{27016});
    EXPECT_FALSE(fired);
}

// A path-filtered callback can be unregistered by its ID.
//   add("controls/sensitivity", cb) → set→count=1 → remove_on_change(id) → set→count=1
TEST_F(JsonConfigurationTest, PathCallbackRemoveById) {
    int count = 0;
    int id = cfg().add_on_change("controls/sensitivity", [&](const std::string&) { ++count; });

    cfg().set_value("controls/sensitivity", std::any{0.5});
    EXPECT_EQ(count, 1);

    cfg().remove_on_change(id);
    cfg().set_value("controls/sensitivity", std::any{0.8});
    EXPECT_EQ(count, 1);
}

TEST_F(JsonConfigurationTest, KeysConsistentWithContains) {
    cfg().set_value("audio/master", std::any{80});
    cfg().set_value("video/fullscreen", std::any{true});

    auto k = cfg().keys();
    for (const auto& key : k)
        EXPECT_TRUE(cfg().contains(key)) << "keys() returned '" << key << "' but contains() is false";
}

// ---------------------------------------------------------------------------
// set_defaults() — first-class defaults support
// ---------------------------------------------------------------------------

// A second mock that uses set_defaults().
class MockJsonConfigWithDefaults : public JsonConfiguration<MockJsonConfigWithDefaults> {
  public:
    void init_defaults() {
        set_defaults({
            {"port", 8080},
            {"name", "default_name"},
            {"verbose", false},
        });
    }
};

class JsonConfigDefaultsTest : public ::testing::Test {
  protected:
    void SetUp() override {
        cfg().clear_on_change();
        cfg().clear();
        cfg().init_defaults();
    }
    static MockJsonConfigWithDefaults& cfg() {
        return MockJsonConfigWithDefaults::instance();
    }
};

TEST_F(JsonConfigDefaultsTest, ValueFallsThroughToDefaults) {
    EXPECT_EQ(cfg().value<int>("port"), 8080);
    EXPECT_EQ(cfg().value<std::string>("name"), "default_name");
    EXPECT_EQ(cfg().value<bool>("verbose"), false);
}

TEST_F(JsonConfigDefaultsTest, SetValueOverridesDefault) {
    cfg().set_value("port", std::any{9090});
    EXPECT_EQ(cfg().value<int>("port"), 9090);
}

TEST_F(JsonConfigDefaultsTest, ContainsIncludesDefaults) {
    EXPECT_TRUE(cfg().contains("port"));
    EXPECT_TRUE(cfg().contains("name"));
    EXPECT_FALSE(cfg().contains("nonexistent"));
}

TEST_F(JsonConfigDefaultsTest, KeysIncludesDefaults) {
    auto k = cfg().keys();
    std::sort(k.begin(), k.end());
    ASSERT_EQ(k.size(), 3u);
    EXPECT_EQ(k[0], "name");
    EXPECT_EQ(k[1], "port");
    EXPECT_EQ(k[2], "verbose");
}

TEST_F(JsonConfigDefaultsTest, SerializeIncludesDefaults) {
    auto bytes = cfg().serialize();
    std::string json(bytes.begin(), bytes.end());
    EXPECT_NE(json.find("8080"), std::string::npos);
    EXPECT_NE(json.find("default_name"), std::string::npos);
}

TEST_F(JsonConfigDefaultsTest, SerializeIncludesOverrides) {
    cfg().set_value("port", std::any{9090});
    auto bytes = cfg().serialize();
    std::string json(bytes.begin(), bytes.end());
    EXPECT_NE(json.find("9090"), std::string::npos);
    EXPECT_EQ(json.find("8080"), std::string::npos); // overridden
}

TEST_F(JsonConfigDefaultsTest, DeserializePreservesDefaults) {
    std::string json = R"({"port": 3000})";
    std::vector<uint8_t> data(json.begin(), json.end());
    ASSERT_TRUE(cfg().deserialize(data));

    // port overridden, name falls through to default
    EXPECT_EQ(cfg().value<int>("port"), 3000);
    EXPECT_EQ(cfg().value<std::string>("name"), "default_name");
}

TEST_F(JsonConfigDefaultsTest, ClearRemovesOverridesButKeepsDefaults) {
    cfg().set_value("port", std::any{9090});
    cfg().clear();
    EXPECT_EQ(cfg().value<int>("port"), 8080); // back to default
}

TEST_F(JsonConfigDefaultsTest, ForEachIncludesDefaults) {
    std::map<std::string, std::any> visited;
    cfg().for_each([&](const std::string& key, const std::any& val) { visited[key] = val; });
    EXPECT_EQ(visited.size(), 3u);
    EXPECT_TRUE(visited.count("port"));
    EXPECT_TRUE(visited.count("name"));
    EXPECT_TRUE(visited.count("verbose"));
}
