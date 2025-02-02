#include "JsonValidation.h"

#include <format>
#include <stdexcept>

void JsonValidation::containsKeys(const nlohmann::json& json_data, const ValidationSettings& settings) {
    for (const auto& [key, value_type] : settings) {

        if (!json_data.contains(key)) {
            throw std::runtime_error(std::format("Missing key {} in json object", key).c_str());
        }

        if (json_data[key].type() != value_type) {
            throw std::runtime_error(std::format("Type mismatch on key {}", key).c_str());
        }
    }
}
