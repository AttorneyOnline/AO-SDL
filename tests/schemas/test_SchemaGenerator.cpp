/// Tests for the schema generator (scripts/generate_schemas.py).
///
/// These exercise the generated C++ output from tests/schemas/test_openapi.yaml,
/// verifying that the generator correctly handles: required/optional fields,
/// nested objects, arrays with $ref items, string enums, additionalProperties
/// (string maps), unconstrained objects, and the YAML boolean trap ("on" key).

#include "utils/JsonSchema.h"

#include <gtest/gtest.h>
#include <json.hpp>

#include <stdexcept>
#include <string>

using json = nlohmann::json;

// Forward-declare the generated lookup functions (prefix: test_schema).
// These are generated from tests/schemas/test_openapi.yaml by the build.
const JsonSchema& test_schema_request_schema(const std::string& operation_id);
const JsonSchema& test_schema_component_schema(const std::string& name);

// -- createWidget: required/optional fields -----------------------------------

TEST(SchemaGeneratorTest, CreateWidgetAcceptsValid) {
    const auto& s = test_schema_request_schema("createWidget");
    EXPECT_EQ(s.validate(json({{"name", "sprocket"}, {"count", 3}})), "");
}

TEST(SchemaGeneratorTest, CreateWidgetAcceptsOptionalLabel) {
    const auto& s = test_schema_request_schema("createWidget");
    EXPECT_EQ(s.validate(json({{"name", "sprocket"}, {"count", 3}, {"label", "A"}})), "");
}

TEST(SchemaGeneratorTest, CreateWidgetRejectsMissingRequired) {
    const auto& s = test_schema_request_schema("createWidget");
    auto err = s.validate(json({{"name", "sprocket"}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("count"), std::string::npos);
}

TEST(SchemaGeneratorTest, CreateWidgetRejectsWrongType) {
    const auto& s = test_schema_request_schema("createWidget");
    auto err = s.validate(json({{"name", "sprocket"}, {"count", "three"}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("count"), std::string::npos);
}

// -- fireEvent: YAML boolean trap ("on" key) ----------------------------------

TEST(SchemaGeneratorTest, FireEventOnFieldSurvivesBooleanTrap) {
    const auto& s = test_schema_request_schema("fireEvent");
    // If the generator mangled "on" -> "True", this would fail
    EXPECT_EQ(s.validate(json({{"name", "click"}, {"on", "start"}})), "");
}

TEST(SchemaGeneratorTest, FireEventRejectsMissingOn) {
    const auto& s = test_schema_request_schema("fireEvent");
    auto err = s.validate(json({{"name", "click"}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("on"), std::string::npos);
}

// -- createGadget: $ref to component schema -----------------------------------

TEST(SchemaGeneratorTest, CreateGadgetAcceptsValid) {
    const auto& s = test_schema_request_schema("createGadget");
    json body = {
        {"id", "g1"},
        {"parts", json::array({{{"name", "bolt"}, {"on", "start"}}})},
    };
    EXPECT_EQ(s.validate(body), "");
}

TEST(SchemaGeneratorTest, CreateGadgetRejectsMissingParts) {
    const auto& s = test_schema_request_schema("createGadget");
    auto err = s.validate(json({{"id", "g1"}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("parts"), std::string::npos);
}

// -- Gadget component: arrays with $ref items ---------------------------------

TEST(SchemaGeneratorTest, GadgetPartsRejectsBadItem) {
    const auto& s = test_schema_component_schema("Gadget");
    json body = {
        {"id", "g1"},
        {"parts", json::array({{{"name", "bolt"}, {"on", 42}}})},
    };
    auto err = s.validate(body);
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("on"), std::string::npos);
}

TEST(SchemaGeneratorTest, GadgetPartsOnFieldNotMangled) {
    const auto& s = test_schema_component_schema("Part");
    EXPECT_EQ(s.validate(json({{"name", "spring"}, {"on", "trigger"}})), "");
}

// -- Config component: enum, additionalProperties, nested object --------------

TEST(SchemaGeneratorTest, ConfigEnumAcceptsValid) {
    const auto& s = test_schema_component_schema("Config");
    EXPECT_EQ(s.validate(json({{"mode", "fast"}})), "");
    EXPECT_EQ(s.validate(json({{"mode", "slow"}})), "");
    EXPECT_EQ(s.validate(json({{"mode", "auto"}})), "");
}

TEST(SchemaGeneratorTest, ConfigEnumRejectsInvalid) {
    const auto& s = test_schema_component_schema("Config");
    auto err = s.validate(json({{"mode", "turbo"}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("turbo"), std::string::npos);
}

TEST(SchemaGeneratorTest, ConfigTagsStringMap) {
    const auto& s = test_schema_component_schema("Config");
    EXPECT_EQ(s.validate(json({{"tags", {{"env", "prod"}, {"region", "us"}}}})), "");

    auto err = s.validate(json({{"tags", {{"env", 42}}}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("env"), std::string::npos);
}

TEST(SchemaGeneratorTest, ConfigNestedObject) {
    const auto& s = test_schema_component_schema("Config");
    EXPECT_EQ(s.validate(json({{"nested", {{"x", 1.5}, {"y", 2.0}}}})), "");

    auto err = s.validate(json({{"nested", {{"x", "bad"}}}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("x"), std::string::npos);
}

// -- submitAction: unconstrained object (no properties on params) -------------

TEST(SchemaGeneratorTest, SubmitActionAcceptsAnyParams) {
    const auto& s = test_schema_request_schema("submitAction");
    json body = {
        {"type", "kick"},
        {"params", {{"reason", "spam"}, {"count", 3}}},
    };
    EXPECT_EQ(s.validate(body), "");
}

TEST(SchemaGeneratorTest, SubmitActionRejectsMissingType) {
    const auto& s = test_schema_request_schema("submitAction");
    auto err = s.validate(json({{"params", json::object()}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("type"), std::string::npos);
}

// -- Unknown operation throws -------------------------------------------------

TEST(SchemaGeneratorTest, UnknownOperationThrows) {
    EXPECT_THROW(test_schema_request_schema("doesNotExist"), std::runtime_error);
}

TEST(SchemaGeneratorTest, UnknownComponentThrows) {
    EXPECT_THROW(test_schema_component_schema("DoesNotExist"), std::runtime_error);
}
