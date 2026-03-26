/**
 * @file NXMessage.h
 * @brief Base type for AONX protocol messages.
 *
 * All v2 messages are JSON objects with a "type" discriminator and a
 * "schema_version" integer. This class wraps the parsed JSON and
 * provides typed access to the common envelope fields.
 *
 * Unlike AO2's AOPacket (which uses positional #-delimited fields),
 * NXMessage is self-describing: unknown fields are ignored, and the
 * schema_version allows forward compatibility per the v2 spec.
 */
#pragma once

#include <json.hpp>

#include <optional>
#include <string>

/// A single AONX protocol message (JSON envelope).
class NXMessage {
  public:
    /// Construct from a pre-parsed JSON object.
    explicit NXMessage(nlohmann::json data);

    /// Parse a raw JSON string into an NXMessage.
    /// Returns nullopt if the string is not valid JSON or missing required fields.
    static std::optional<NXMessage> deserialize(const std::string& raw);

    /// Serialize this message to a JSON string.
    std::string serialize() const;

    /// The message type discriminator (e.g. "ic_message", "moderation_action").
    const std::string& type() const {
        return type_;
    }

    /// Per-schema version number.
    int schema_version() const {
        return schema_version_;
    }

    /// Access the full JSON payload for field extraction.
    const nlohmann::json& data() const {
        return data_;
    }

    /// Convenience: extract a typed field with a default fallback.
    template <typename T>
    T field(const std::string& key, const T& fallback = T{}) const {
        auto it = data_.find(key);
        if (it == data_.end())
            return fallback;
        try {
            return it->get<T>();
        }
        catch (...) {
            return fallback;
        }
    }

  private:
    nlohmann::json data_;
    std::string type_;
    int schema_version_ = 0;
};
