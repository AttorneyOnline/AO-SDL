#include "utils/JsonSchema.h"

#include <format>

// -- Leaf factories -----------------------------------------------------------

JsonSchema JsonSchema::string_type() {
    JsonSchema s;
    s.type_ = Type::string_t;
    return s;
}

JsonSchema JsonSchema::integer_type() {
    JsonSchema s;
    s.type_ = Type::integer_t;
    return s;
}

JsonSchema JsonSchema::number_type() {
    JsonSchema s;
    s.type_ = Type::number_t;
    return s;
}

JsonSchema JsonSchema::boolean_type() {
    JsonSchema s;
    s.type_ = Type::boolean_t;
    return s;
}

JsonSchema JsonSchema::string_enum(std::vector<std::string> allowed) {
    JsonSchema s;
    s.type_ = Type::string_enum_t;
    s.allowed_values_ = std::move(allowed);
    return s;
}

// -- Composite factories ------------------------------------------------------

JsonSchema JsonSchema::array(JsonSchema item_schema) {
    JsonSchema s;
    s.type_ = Type::array_t;
    s.item_schema_ = std::make_shared<JsonSchema>(std::move(item_schema));
    return s;
}

JsonSchema JsonSchema::string_map(JsonSchema value_schema) {
    JsonSchema s;
    s.type_ = Type::string_map_t;
    s.item_schema_ = std::make_shared<JsonSchema>(std::move(value_schema));
    return s;
}

JsonSchema JsonSchema::one_of(std::vector<JsonSchema> variants) {
    JsonSchema s;
    s.type_ = Type::one_of_t;
    for (auto& v : variants)
        s.variants_.push_back(std::make_shared<JsonSchema>(std::move(v)));
    return s;
}

// -- Object builder -----------------------------------------------------------

JsonSchema::ObjectBuilder JsonSchema::object() {
    return ObjectBuilder{};
}

JsonSchema::ObjectBuilder& JsonSchema::ObjectBuilder::required(std::string name, JsonSchema schema) {
    fields_.push_back({std::move(name), std::make_shared<JsonSchema>(std::move(schema)), true});
    return *this;
}

JsonSchema::ObjectBuilder& JsonSchema::ObjectBuilder::optional(std::string name, JsonSchema schema) {
    fields_.push_back({std::move(name), std::make_shared<JsonSchema>(std::move(schema)), false});
    return *this;
}

JsonSchema JsonSchema::ObjectBuilder::build() {
    JsonSchema s;
    s.type_ = Type::object_t;
    for (auto& f : fields_) {
        s.fields_.push_back({std::move(f.name), std::move(f.schema), f.is_required});
    }
    return s;
}

// -- Validation ---------------------------------------------------------------

std::string JsonSchema::validate(const nlohmann::json& value) const {
    return validate_impl(value, "");
}

static const char* json_type_name(const nlohmann::json& v) {
    switch (v.type()) {
    case nlohmann::json::value_t::null:
        return "null";
    case nlohmann::json::value_t::boolean:
        return "boolean";
    case nlohmann::json::value_t::number_integer:
        return "integer";
    case nlohmann::json::value_t::number_unsigned:
        return "integer";
    case nlohmann::json::value_t::number_float:
        return "number";
    case nlohmann::json::value_t::string:
        return "string";
    case nlohmann::json::value_t::array:
        return "array";
    case nlohmann::json::value_t::object:
        return "object";
    default:
        return "unknown";
    }
}

static std::string field_path(const std::string& base, const std::string& field) {
    if (base.empty())
        return field;
    return base + "." + field;
}

static std::string index_path(const std::string& base, size_t i) {
    return std::format("{}[{}]", base, i);
}

std::string JsonSchema::validate_impl(const nlohmann::json& value, const std::string& path) const {
    auto err = [&](const std::string& msg) -> std::string {
        if (path.empty())
            return msg;
        return path + ": " + msg;
    };

    switch (type_) {
    case Type::none:
        return {};

    case Type::string_t: {
        if (!value.is_string())
            return err(std::format("expected string, got {}", json_type_name(value)));
        auto len = static_cast<int>(value.get_ref<const std::string&>().size());
        if (min_length_ > 0 && len < min_length_)
            return err(std::format("string length {} is below minimum {}", len, min_length_));
        if (max_length_ > 0 && len > max_length_)
            return err(std::format("string length {} exceeds maximum {}", len, max_length_));
        return {};
    }

    case Type::integer_t: {
        if (!value.is_number_integer() && !value.is_number_unsigned())
            return err(std::format("expected integer, got {}", json_type_name(value)));
        if (has_range_) {
            auto v = value.get<double>();
            if (v < minimum_)
                return err(std::format("value {} is below minimum {}", v, minimum_));
            if (v > maximum_)
                return err(std::format("value {} exceeds maximum {}", v, maximum_));
        }
        return {};
    }

    case Type::number_t:
        if (!value.is_number())
            return err(std::format("expected number, got {}", json_type_name(value)));
        return {};

    case Type::boolean_t:
        if (!value.is_boolean())
            return err(std::format("expected boolean, got {}", json_type_name(value)));
        return {};

    case Type::string_enum_t: {
        if (!value.is_string())
            return err(std::format("expected string, got {}", json_type_name(value)));
        auto& s = value.get_ref<const std::string&>();
        for (const auto& a : allowed_values_) {
            if (s == a)
                return {};
        }
        std::string allowed_list;
        for (size_t i = 0; i < allowed_values_.size(); ++i) {
            if (i > 0)
                allowed_list += ", ";
            allowed_list += "\"" + allowed_values_[i] + "\"";
        }
        return err(std::format("\"{}\" is not one of [{}]", s, allowed_list));
    }

    case Type::array_t: {
        if (!value.is_array())
            return err(std::format("expected array, got {}", json_type_name(value)));
        for (size_t i = 0; i < value.size(); ++i) {
            auto result = item_schema_->validate_impl(value[i], index_path(path, i));
            if (!result.empty())
                return result;
        }
        return {};
    }

    case Type::string_map_t: {
        if (!value.is_object())
            return err(std::format("expected object, got {}", json_type_name(value)));
        for (const auto& [key, val] : value.items()) {
            auto result = item_schema_->validate_impl(val, field_path(path, key));
            if (!result.empty())
                return result;
        }
        return {};
    }

    case Type::object_t: {
        if (!value.is_object())
            return err(std::format("expected object, got {}", json_type_name(value)));
        for (const auto& field : fields_) {
            auto fp = field_path(path, field.name);
            if (!value.contains(field.name)) {
                if (field.is_required)
                    return err(std::format("missing required field \"{}\"", field.name));
                continue;
            }
            auto result = field.schema->validate_impl(value[field.name], fp);
            if (!result.empty())
                return result;
        }
        return {};
    }

    case Type::one_of_t: {
        int matches = 0;
        std::string last_error;
        for (const auto& variant : variants_) {
            auto result = variant->validate_impl(value, path);
            if (result.empty())
                ++matches;
            else
                last_error = result;
        }
        if (matches == 1)
            return {};
        if (matches == 0)
            return err(std::format("value does not match any oneOf variant (last error: {})", last_error));
        return err(std::format("value matches {} oneOf variants, expected exactly 1", matches));
    }
    }

    return {};
}
