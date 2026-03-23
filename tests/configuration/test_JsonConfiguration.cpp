#include "configuration/UserConfiguration.h"

#include <gtest/gtest.h>

#include <any>
#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Fixture — resets the UserConfiguration singleton between tests.
// ---------------------------------------------------------------------------

class JsonConfigurationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        cfg().set_on_change(nullptr);
        cfg().clear();
    }

    static UserConfiguration& cfg() { return UserConfiguration::instance(); }
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
    EXPECT_NO_THROW((void)nlohmann::json::parse(json_str));

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

TEST_F(JsonConfigurationTest, CallbackOnDeserialize) {
    bool fired = false;
    cfg().set_on_change([&](const std::string&) { fired = true; });

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
