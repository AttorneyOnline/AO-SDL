#include "JsonValidation.h"

#include "utils/Log.h"

bool JsonValidation::containsKeys(nlohmann::json json_data, ValidationSettings settings) {
    for (const auto& [key, value_type] : settings) {

        if (!json_data.contains(key)) {
            Log::log_print(DEBUG, std::format("Missing key {} in object", key).c_str());
            return false;
        }

        if (json_data[key].type() != value_type) {
            Log::log_print(DEBUG, std::format("Type mismatch on key {}", key).c_str());
            return false;
        }
    }
    return true;
}
