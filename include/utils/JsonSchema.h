/**
 * @file JsonSchema.h
 * @brief Declarative JSON schema validator for REST endpoint request bodies.
 *
 * Schemas are built via a fluent API and validate nlohmann::json values,
 * returning dot-path error messages (e.g. "objects[2].transform.x: expected
 * number, got string"). Designed for code generation from OpenAPI specs.
 */
#pragma once

#include <json.hpp>

#include <memory>
#include <string>
#include <vector>

/// Validates a JSON value against a declarative schema.
/// Immutable after construction — safe to declare as static const.
class JsonSchema {
  public:
    JsonSchema() = default;

    /// Validate a JSON value. Returns empty string on success, or a
    /// human-readable error message with dot-path location on failure.
    std::string validate(const nlohmann::json& value) const;

    // -- Leaf type factories --------------------------------------------------

    static JsonSchema string_type();
    static JsonSchema integer_type();
    static JsonSchema number_type();
    static JsonSchema boolean_type();

    /// String constrained to a set of allowed values.
    static JsonSchema string_enum(std::vector<std::string> allowed);

    // -- Composite type factories ---------------------------------------------

    /// Array where each element must match item_schema.
    static JsonSchema array(JsonSchema item_schema);

    /// Object with unconstrained keys, each value matching value_schema.
    /// Used for OpenAPI additionalProperties (e.g. states: map<string,string>).
    static JsonSchema string_map(JsonSchema value_schema);

    // -- Object builder -------------------------------------------------------

    /// Start building an object schema.
    class ObjectBuilder {
      public:
        ObjectBuilder& required(std::string name, JsonSchema schema);
        ObjectBuilder& optional(std::string name, JsonSchema schema);

        /// Finalize and return the schema.
        JsonSchema build();

      private:
        struct Field {
            std::string name;
            std::shared_ptr<JsonSchema> schema;
            bool is_required;
        };
        std::vector<Field> fields_;
    };

    static ObjectBuilder object();

  private:
    enum class Type {
        none,
        string_t,
        integer_t,
        number_t,
        boolean_t,
        string_enum_t,
        array_t,
        object_t,
        string_map_t,
    };

    struct Field {
        std::string name;
        std::shared_ptr<JsonSchema> schema;
        bool is_required;
    };

    std::string validate_impl(const nlohmann::json& value, const std::string& path) const;

    Type type_ = Type::none;
    std::vector<std::string> allowed_values_; // string_enum
    std::shared_ptr<JsonSchema> item_schema_; // array, string_map
    std::vector<Field> fields_;               // object
};
