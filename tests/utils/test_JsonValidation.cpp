#include "utils/JsonValidation.h"

#include <gtest/gtest.h>
#include <json.hpp>
#include <stdexcept>

using json = nlohmann::json;
using namespace JsonValidation;

TEST(JsonValidation, PassesWhenAllKeysPresent) {
    json j = {{"name", "foo"}, {"count", 3u}};
    ValidationSettings settings = {
        {"name", ValueType::string},
        {"count", ValueType::number_unsigned},
    };
    EXPECT_NO_THROW(containsKeys(j, settings));
}

TEST(JsonValidation, ThrowsOnMissingKey) {
    json j = {{"name", "foo"}};
    ValidationSettings settings = {
        {"name", ValueType::string},
        {"missing", ValueType::number_unsigned},
    };
    EXPECT_THROW(containsKeys(j, settings), std::runtime_error);
}

TEST(JsonValidation, ThrowsOnTypeMismatch) {
    json j = {{"name", 42}}; // integer, not string
    ValidationSettings settings = {{"name", ValueType::string}};
    EXPECT_THROW(containsKeys(j, settings), std::runtime_error);
}

TEST(JsonValidation, EmptySettingsAlwaysPasses) {
    json j = {{"anything", "here"}};
    ValidationSettings settings;
    EXPECT_NO_THROW(containsKeys(j, settings));
}

TEST(JsonValidation, PassesForNumberSignedType) {
    json j = {{"value", -5}};
    ValidationSettings settings = {{"value", ValueType::number_integer}};
    EXPECT_NO_THROW(containsKeys(j, settings));
}

TEST(JsonValidation, ThrowsWhenSignedIntPassedAsUnsigned) {
    json j = {{"value", -5}};
    ValidationSettings settings = {{"value", ValueType::number_unsigned}};
    EXPECT_THROW(containsKeys(j, settings), std::runtime_error);
}
