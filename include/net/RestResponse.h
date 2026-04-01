#pragma once

#include <json.hpp>

#include <string>

/// Response from a REST endpoint handler. RestRouter translates this
/// into an http::Response.
struct RestResponse {
    int status = 200;
    nlohmann::json body;
    std::string content_type = "application/json";

    static RestResponse json(int status_code, nlohmann::json json_body) {
        return {status_code, std::move(json_body), "application/json"};
    }

    static RestResponse error(int status_code, const std::string& reason) {
        return {status_code, {{"reason", reason}}, "application/json"};
    }

    static RestResponse no_content() {
        return {204, nullptr, ""};
    }
};
