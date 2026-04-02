#include "net/RestRouter.h"

#include "game/ServerSession.h"
#include "utils/Log.h"

#include "net/Http.h"

// -- RestRouter --------------------------------------------------------------

void RestRouter::set_auth_func(AuthFunc func) {
    auth_func_ = std::move(func);
}

void RestRouter::register_endpoint(std::unique_ptr<RestEndpoint> endpoint) {
    endpoints_.push_back(std::move(endpoint));
}

void RestRouter::set_cors_origin(const std::string& origin) {
    cors_origin_ = origin;
}

void RestRouter::bind(http::Server& server) {
    // Apply CORS headers to all responses, including httplib's built-in
    // 404 for unmatched routes (which bypasses our dispatch method).
    if (!cors_origin_.empty()) {
        server.set_default_headers({
            {"Access-Control-Allow-Origin", cors_origin_},
            {"Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
        });
    }

    for (auto& ep : endpoints_) {
        auto* raw_ep = ep.get();
        auto handler = [this, raw_ep](const http::Request& req, http::Response& res) { dispatch(*raw_ep, req, res); };

        const auto& method = raw_ep->method();
        const auto& pattern = raw_ep->path_pattern();

        if (method == "GET")
            server.Get(pattern, handler);
        else if (method == "POST")
            server.Post(pattern, handler);
        else if (method == "PUT")
            server.Put(pattern, handler);
        else if (method == "PATCH")
            server.Patch(pattern, handler);
        else if (method == "DELETE")
            server.Delete(pattern, handler);
        else
            Log::log_print(ERR, "REST: unknown method '%s' for %s", method.c_str(), pattern.c_str());

        // Preflight handler for this route
        // CORS headers are already applied by set_default_headers.
        server.Options(pattern, [](const http::Request&, http::Response& res) { res.status = 204; });
    }

    // Catch-all preflight handler for unmatched routes. Without this,
    // OPTIONS to a non-existent path returns 404, which browsers treat
    // as a failed preflight and block the actual request.
    if (!cors_origin_.empty()) {
        server.Options(".*", [](const http::Request&, http::Response& res) { res.status = 204; });
    }
}

void RestRouter::dispatch(RestEndpoint& endpoint, const http::Request& req, http::Response& res) {
    // CORS headers are set globally via set_default_headers in bind().
    try {
        // Build RestRequest from http::Request (no lock needed — pure parsing)
        RestRequest rest_req;
        rest_req.method = req.method;
        rest_req.path = req.path;

        for (auto& [key, value] : req.path_params)
            rest_req.path_params[key] = value;

        for (auto& [key, value] : req.params)
            rest_req.query_params[key] = value;

        // Parse JSON body if present
        auto content_type = req.get_header_value("Content-Type");
        if (!req.body.empty() && content_type.find("application/json") != std::string::npos) {
            try {
                rest_req.body = nlohmann::json::parse(req.body);
            }
            catch (const nlohmann::json::parse_error&) {
                res.status = 400;
                res.set_content(R"({"reason":"Malformed JSON in request body"})", "application/json");
                return;
            }
        }

        // Extract bearer token
        auto auth_header = req.get_header_value("Authorization");
        if (auth_header.size() > 7 && auth_header.substr(0, 7) == "Bearer ") {
            rest_req.bearer_token = auth_header.substr(7);
        }

        // Single lock for auth + handler — prevents session from being
        // destroyed between the auth check and the handler call.
        RestResponse rest_res;
        {
            std::lock_guard lock(dispatch_mutex_);

            if (endpoint.requires_auth()) {
                if (rest_req.bearer_token.empty()) {
                    res.status = 401;
                    res.set_content(R"({"reason":"Missing session token"})", "application/json");
                    return;
                }
                if (!auth_func_) {
                    res.status = 500;
                    res.set_content(R"({"reason":"Server authentication is not configured"})", "application/json");
                    return;
                }

                rest_req.session = auth_func_(rest_req.bearer_token);
                if (!rest_req.session) {
                    res.status = 401;
                    res.set_content(R"({"reason":"Invalid or expired session token"})", "application/json");
                    return;
                }
                rest_req.session->touch();
            }

            rest_res = endpoint.handle(rest_req);
        }

        // Write response (outside lock — no game state access)
        res.status = rest_res.status;
        if (rest_res.status == 204 || rest_res.body.is_null()) {
            // No content
        }
        else {
            res.set_content(rest_res.body.dump(), rest_res.content_type);
        }
    }
    catch (const std::exception& e) {
        Log::log_print(ERR, "REST: exception in %s %s: %s", endpoint.method().c_str(), endpoint.path_pattern().c_str(),
                       e.what());
        res.status = 500;
        res.set_content(R"({"reason":"An internal server error occurred"})", "application/json");
    }
}
