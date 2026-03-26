#include "NXMessage.h"

NXMessage::NXMessage(nlohmann::json data) : data_(std::move(data)) {
    if (data_.contains("type") && data_["type"].is_string())
        type_ = data_["type"].get<std::string>();

    if (data_.contains("schema_version") && data_["schema_version"].is_number_integer())
        schema_version_ = data_["schema_version"].get<int>();
}

std::optional<NXMessage> NXMessage::deserialize(const std::string& raw) {
    auto parsed = nlohmann::json::parse(raw, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object())
        return std::nullopt;

    // Every AONX message must have "type" and "schema_version"
    if (!parsed.contains("type") || !parsed.contains("schema_version"))
        return std::nullopt;

    return NXMessage(std::move(parsed));
}

std::string NXMessage::serialize() const {
    return data_.dump();
}
