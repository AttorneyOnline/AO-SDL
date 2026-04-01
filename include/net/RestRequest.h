#pragma once

#include <json.hpp>

#include <optional>
#include <string>
#include <unordered_map>

struct ServerSession;

/// Protocol-agnostic representation of an incoming REST request.
/// Built by RestRouter from http::Request before dispatching to handlers.
struct RestRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> path_params;
    std::unordered_map<std::string, std::string> query_params;
    std::optional<nlohmann::json> body;
    std::string bearer_token;
    ServerSession* session = nullptr; ///< Non-null after successful auth.
};
