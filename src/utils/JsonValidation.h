#ifndef JSONVALIDATION_H
#define JSONVALIDATION_H

#include "utils/json.hpp"

namespace JsonValidation {
using ValueType = nlohmann::json::value_t;
using ValidationSettings = std::map<std::string, ValueType>;

bool containsKeys(nlohmann::json json_data, ValidationSettings settings);

}; // namespace JsonValidation

#endif // JSONVALIDATION_H
