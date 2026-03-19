/**
 * @file JsonValidation.h
 * @brief Validates JSON objects against expected key/type schemas.
 */
#pragma once

#include <json.hpp>

/**
 * @brief Utilities for validating JSON objects against expected schemas.
 */
namespace JsonValidation {

/** @brief Alias for the nlohmann JSON value type enumeration. */
using ValueType = nlohmann::json::value_t;

/**
 * @brief A schema mapping expected key names to their required JSON types.
 *
 * Each entry maps a string key to the nlohmann::json::value_t that the
 * corresponding value must have.
 */
using ValidationSettings = std::map<std::string, ValueType>;

/**
 * @brief Validates that a JSON object contains the specified keys with
 *        the expected types.
 *
 * Iterates over @p settings and checks that each key exists in
 * @p json_data and that its value matches the expected type.
 *
 * @param json_data The JSON object to validate.
 * @param settings  Map of required key names to their expected value types.
 * @throws std::runtime_error (or similar) if a required key is missing or
 *         has the wrong type.
 */
void containsKeys(const nlohmann::json& json_data, const ValidationSettings& settings);

}; // namespace JsonValidation
