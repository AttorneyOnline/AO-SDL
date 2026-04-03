#include "utils/JsonSchema.h"

#include <gtest/gtest.h>
#include <json.hpp>

using json = nlohmann::json;

// -- Leaf type tests ----------------------------------------------------------

TEST(JsonSchemaTest, StringTypeAcceptsStrings) {
    auto s = JsonSchema::string_type();
    EXPECT_EQ(s.validate(json("hello")), "");
    EXPECT_EQ(s.validate(json("")), "");
}

TEST(JsonSchemaTest, StringTypeRejectsNonStrings) {
    auto s = JsonSchema::string_type();
    EXPECT_NE(s.validate(json(42)), "");
    EXPECT_NE(s.validate(json(true)), "");
    EXPECT_NE(s.validate(json(nullptr)), "");
}

TEST(JsonSchemaTest, IntegerTypeAcceptsIntegers) {
    auto s = JsonSchema::integer_type();
    EXPECT_EQ(s.validate(json(42)), "");
    EXPECT_EQ(s.validate(json(-1)), "");
    EXPECT_EQ(s.validate(json(0u)), "");
}

TEST(JsonSchemaTest, IntegerTypeRejectsFloats) {
    auto s = JsonSchema::integer_type();
    EXPECT_NE(s.validate(json(3.14)), "");
}

TEST(JsonSchemaTest, NumberTypeAcceptsAllNumbers) {
    auto s = JsonSchema::number_type();
    EXPECT_EQ(s.validate(json(42)), "");
    EXPECT_EQ(s.validate(json(3.14)), "");
    EXPECT_EQ(s.validate(json(-1)), "");
}

TEST(JsonSchemaTest, NumberTypeRejectsNonNumbers) {
    auto s = JsonSchema::number_type();
    EXPECT_NE(s.validate(json("42")), "");
}

TEST(JsonSchemaTest, BooleanTypeAcceptsBooleans) {
    auto s = JsonSchema::boolean_type();
    EXPECT_EQ(s.validate(json(true)), "");
    EXPECT_EQ(s.validate(json(false)), "");
}

TEST(JsonSchemaTest, BooleanTypeRejectsNonBooleans) {
    auto s = JsonSchema::boolean_type();
    EXPECT_NE(s.validate(json(1)), "");
    EXPECT_NE(s.validate(json("true")), "");
}

// -- String length constraints ------------------------------------------------

TEST(JsonSchemaTest, MinLengthRejectsEmpty) {
    auto s = JsonSchema::string_type().min_length(1);
    EXPECT_NE(s.validate(json("")), "");
    EXPECT_EQ(s.validate(json("a")), "");
}

TEST(JsonSchemaTest, MaxLengthRejectsTooLong) {
    auto s = JsonSchema::string_type().max_length(3);
    EXPECT_EQ(s.validate(json("abc")), "");
    EXPECT_NE(s.validate(json("abcd")), "");
}

// -- Integer range constraints ------------------------------------------------

TEST(JsonSchemaTest, IntegerMinimumRejectsBelowRange) {
    auto s = JsonSchema::integer_type().minimum(0).maximum(10);
    EXPECT_EQ(s.validate(json(0)), "");
    EXPECT_EQ(s.validate(json(10)), "");
    EXPECT_NE(s.validate(json(-1)), "");
    EXPECT_NE(s.validate(json(11)), "");
}

// -- String enum tests --------------------------------------------------------

TEST(JsonSchemaTest, StringEnumAcceptsAllowed) {
    auto s = JsonSchema::string_enum({"linear", "quad_in", "quad_out"});
    EXPECT_EQ(s.validate(json("linear")), "");
    EXPECT_EQ(s.validate(json("quad_in")), "");
}

TEST(JsonSchemaTest, StringEnumRejectsDisallowed) {
    auto s = JsonSchema::string_enum({"linear", "quad_in"});
    auto err = s.validate(json("cubic"));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("cubic"), std::string::npos);
    EXPECT_NE(err.find("linear"), std::string::npos);
}

TEST(JsonSchemaTest, StringEnumRejectsNonString) {
    auto s = JsonSchema::string_enum({"a", "b"});
    EXPECT_NE(s.validate(json(42)), "");
}

// -- Array tests --------------------------------------------------------------

TEST(JsonSchemaTest, ArrayAcceptsValidItems) {
    auto s = JsonSchema::array(JsonSchema::integer_type());
    EXPECT_EQ(s.validate(json::array({1, 2, 3})), "");
    EXPECT_EQ(s.validate(json::array()), "");
}

TEST(JsonSchemaTest, ArrayRejectsInvalidItems) {
    auto s = JsonSchema::array(JsonSchema::integer_type());
    auto err = s.validate(json::array({1, "bad", 3}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("[1]"), std::string::npos);
}

TEST(JsonSchemaTest, ArrayRejectsNonArray) {
    auto s = JsonSchema::array(JsonSchema::string_type());
    EXPECT_NE(s.validate(json("not array")), "");
}

// -- String map tests ---------------------------------------------------------

TEST(JsonSchemaTest, StringMapAcceptsValidEntries) {
    auto s = JsonSchema::string_map(JsonSchema::string_type());
    EXPECT_EQ(s.validate(json({{"a", "1"}, {"b", "2"}})), "");
    EXPECT_EQ(s.validate(json::object()), "");
}

TEST(JsonSchemaTest, StringMapRejectsInvalidValues) {
    auto s = JsonSchema::string_map(JsonSchema::string_type());
    auto err = s.validate(json({{"a", "ok"}, {"b", 42}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("b"), std::string::npos);
}

// -- Object tests -------------------------------------------------------------

TEST(JsonSchemaTest, ObjectAcceptsValidRequiredFields) {
    auto s = JsonSchema::object()
                 .required("name", JsonSchema::string_type())
                 .required("count", JsonSchema::integer_type())
                 .build();
    EXPECT_EQ(s.validate(json({{"name", "foo"}, {"count", 5}})), "");
}

TEST(JsonSchemaTest, ObjectRejectsMissingRequired) {
    auto s = JsonSchema::object()
                 .required("name", JsonSchema::string_type())
                 .required("count", JsonSchema::integer_type())
                 .build();
    auto err = s.validate(json({{"name", "foo"}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("count"), std::string::npos);
}

TEST(JsonSchemaTest, ObjectAcceptsMissingOptional) {
    auto s = JsonSchema::object()
                 .required("name", JsonSchema::string_type())
                 .optional("label", JsonSchema::string_type())
                 .build();
    EXPECT_EQ(s.validate(json({{"name", "foo"}})), "");
}

TEST(JsonSchemaTest, ObjectRejectsWrongTypeOnOptional) {
    auto s = JsonSchema::object().optional("count", JsonSchema::integer_type()).build();
    auto err = s.validate(json({{"count", "not a number"}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("count"), std::string::npos);
}

TEST(JsonSchemaTest, ObjectIgnoresExtraFields) {
    auto s = JsonSchema::object().required("name", JsonSchema::string_type()).build();
    // Extra field "extra" should not cause an error
    EXPECT_EQ(s.validate(json({{"name", "foo"}, {"extra", 99}})), "");
}

TEST(JsonSchemaTest, ObjectRejectsNonObject) {
    auto s = JsonSchema::object().required("name", JsonSchema::string_type()).build();
    EXPECT_NE(s.validate(json("string")), "");
    EXPECT_NE(s.validate(json(42)), "");
}

// -- Nested object tests ------------------------------------------------------

TEST(JsonSchemaTest, NestedObjectValidation) {
    auto s = JsonSchema::object()
                 .required("transform", JsonSchema::object()
                                            .required("x", JsonSchema::number_type())
                                            .required("y", JsonSchema::number_type())
                                            .build())
                 .build();

    EXPECT_EQ(s.validate(json({{"transform", {{"x", 1.0}, {"y", 2.0}}}})), "");

    auto err = s.validate(json({{"transform", {{"x", "bad"}, {"y", 2.0}}}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("transform.x"), std::string::npos);
}

TEST(JsonSchemaTest, NestedMissingRequired) {
    auto s =
        JsonSchema::object()
            .required("animation",
                      JsonSchema::object()
                          .required("keyframes",
                                    JsonSchema::array(
                                        JsonSchema::object().required("time_ms", JsonSchema::integer_type()).build()))
                          .build())
            .build();

    auto err = s.validate(json({{"animation", json::object()}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("keyframes"), std::string::npos);
}

// -- Dot-path error reporting -------------------------------------------------

TEST(JsonSchemaTest, DotPathInArrayOfObjects) {
    auto s = JsonSchema::array(JsonSchema::object()
                                   .required("id", JsonSchema::string_type())
                                   .required("value", JsonSchema::integer_type())
                                   .build());

    json input = json::array({
        {{"id", "a"}, {"value", 1}},
        {{"id", "b"}, {"value", "not_int"}},
    });

    auto err = s.validate(input);
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("[1]"), std::string::npos);
    EXPECT_NE(err.find("value"), std::string::npos);
}

// -- Complex schema (mirrors IcMessage structure) -----------------------------

TEST(JsonSchemaTest, ComplexNestedSchemaValidates) {
    // Simplified IcMessage-like schema
    auto schema =
        JsonSchema::object()
            .optional("objects",
                      JsonSchema::array(JsonSchema::object()
                                            .required("id", JsonSchema::string_type())
                                            .required("z", JsonSchema::integer_type())
                                            .required("visible", JsonSchema::boolean_type())
                                            .optional("states", JsonSchema::string_map(JsonSchema::string_type()))
                                            .optional("transform", JsonSchema::object()
                                                                       .optional("x", JsonSchema::number_type())
                                                                       .optional("y", JsonSchema::number_type())
                                                                       .build())
                                            .build()))
            .optional("text", JsonSchema::array(JsonSchema::object()
                                                    .required("id", JsonSchema::string_type())
                                                    .required("content", JsonSchema::string_type())
                                                    .required("on", JsonSchema::string_type())
                                                    .build()))
            .build();

    // Valid input
    json valid = {
        {"objects",
         {{{"id", "char1"},
           {"z", 5},
           {"visible", true},
           {"states", {{"idle", "abc123"}}},
           {"transform", {{"x", 100.0}}}}}},
        {"text", {{{"id", "t1"}, {"content", "Hello!"}, {"on", "start"}}}},
    };
    EXPECT_EQ(schema.validate(valid), "");

    // Invalid: missing required 'visible' in object
    json invalid = {
        {"objects", {{{"id", "char1"}, {"z", 5}}}},
    };
    auto err = schema.validate(invalid);
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("visible"), std::string::npos);
}

// -- oneOf tests --------------------------------------------------------------

TEST(JsonSchemaTest, OneOfAcceptsExactlyOneMatch) {
    auto s = JsonSchema::one_of({
        JsonSchema::object()
            .required("char_id", JsonSchema::string_type())
            .required("available", JsonSchema::boolean_type())
            .build(),
        JsonSchema::object().required("taken", JsonSchema::array(JsonSchema::integer_type())).build(),
    });

    // Individual shape
    EXPECT_EQ(s.validate(json({{"char_id", "abc"}, {"available", false}})), "");

    // Bulk shape
    EXPECT_EQ(s.validate(json({{"taken", {0, 1, 0}}})), "");
}

TEST(JsonSchemaTest, OneOfRejectsNoMatch) {
    auto s = JsonSchema::one_of({
        JsonSchema::object().required("a", JsonSchema::string_type()).build(),
        JsonSchema::object().required("b", JsonSchema::integer_type()).build(),
    });

    auto err = s.validate(json({{"c", true}}));
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("oneOf"), std::string::npos);
}

TEST(JsonSchemaTest, OneOfRejectsMultipleMatches) {
    // Both variants accept any object (no required fields)
    auto s = JsonSchema::one_of({
        JsonSchema::object().optional("a", JsonSchema::string_type()).build(),
        JsonSchema::object().optional("b", JsonSchema::string_type()).build(),
    });

    auto err = s.validate(json::object());
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("2 oneOf variants"), std::string::npos);
}

// -- Default (empty) schema accepts anything ----------------------------------

TEST(JsonSchemaTest, DefaultSchemaAcceptsAnything) {
    JsonSchema s;
    EXPECT_EQ(s.validate(json(42)), "");
    EXPECT_EQ(s.validate(json("str")), "");
    EXPECT_EQ(s.validate(json::object()), "");
    EXPECT_EQ(s.validate(json::array()), "");
}

// -- Generated schemas (only when AOSDL_HAS_GENERATED_SCHEMAS is defined) -----

#ifdef AOSDL_HAS_GENERATED_SCHEMAS
#include "utils/GeneratedSchemas.h"

TEST(GeneratedSchemasTest, CreateSessionSchemaRejectsEmpty) {
    const auto& schema = aonx_request_schema("createSession");
    auto err = schema.validate(json::object());
    EXPECT_NE(err, "");
    // Should report one of the required fields (hdid, client_name, client_version)
    EXPECT_NE(err.find("missing required field"), std::string::npos);
}

TEST(GeneratedSchemasTest, CreateSessionSchemaAcceptsValid) {
    const auto& schema = aonx_request_schema("createSession");
    json body = {
        {"client_name", "AO-SDL"},
        {"client_version", "1.0.0"},
        {"hdid", "abc123"},
    };
    EXPECT_EQ(schema.validate(body), "");
}

TEST(GeneratedSchemasTest, CreateSessionSchemaRejectsWrongType) {
    const auto& schema = aonx_request_schema("createSession");
    json body = {
        {"client_name", 42},
        {"client_version", "1.0.0"},
        {"hdid", "abc123"},
    };
    auto err = schema.validate(body);
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("client_name"), std::string::npos);
}

TEST(GeneratedSchemasTest, IcMessageSchemaAcceptsValid) {
    const auto& schema = aonx_component_schema("IcMessage");
    json ic = {
        {"schema_version", 1},
        {"objects", {{{"id", "def"}, {"z", 0}, {"visible", true}}}},
        {"text", {{{"id", "t1"}, {"content", "Hello"}, {"on", "start"}}}},
    };
    EXPECT_EQ(schema.validate(ic), "");
}

TEST(GeneratedSchemasTest, IcMessageSchemaRejectsInvalidObject) {
    const auto& schema = aonx_component_schema("IcMessage");
    json ic = {
        {"objects", {{{"id", "char"}, {"z", "not_int"}, {"visible", true}}}},
    };
    auto err = schema.validate(ic);
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("z"), std::string::npos);
}

TEST(GeneratedSchemasTest, SetHealthBarSchemaValidatesEnum) {
    const auto& schema = aonx_request_schema("setHealthBar");
    json valid = {{"side", "defense"}, {"value", 5}};
    EXPECT_EQ(schema.validate(valid), "");

    json invalid = {{"side", "invalid_side"}, {"value", 5}};
    auto err = schema.validate(invalid);
    EXPECT_NE(err, "");
    EXPECT_NE(err.find("invalid_side"), std::string::npos);
}

TEST(GeneratedSchemasTest, UnknownOperationThrows) {
    EXPECT_THROW(aonx_request_schema("nonexistent"), std::runtime_error);
}

#endif // AOSDL_HAS_GENERATED_SCHEMAS
