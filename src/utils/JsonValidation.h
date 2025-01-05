#ifndef JSONVALIDATION_H
#define JSONVALIDATION_H

#include "utils/json.hpp"

namespace JsonValidation {
using ValueType = nlohmann::json::value_t;
using ValidationSettings = std::map<std::string, ValueType>;

void containsKeys(const nlohmann::json& json_data, const ValidationSettings& settings);

}; // namespace JsonValidation

#endif // JSONVALIDATION_H
