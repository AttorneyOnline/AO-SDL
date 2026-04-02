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

void RestRouter::set_cors_origins(std::vector<std::string> origins) {
    cors_wildcard_ = false;
    cors_origins_.clear();
    for (auto& o : origins) {
        if (o == "*") {
            cors_wildcard_ = true;
            cors_origins_.clear();
            return;
        }
        cors_origins_.push_back(std::move(o));
    }
}

bool RestRouter::cors_enabled() const {
    return cors_wildcard_ || !cors_origins_.empty();
}

void RestRouter::apply_cors_origin(const http::Request& req, http::Response& res) const {
    // Multi-origin mode: echo back the Origin if it matches the allow-list.
    // Methods, Headers, and Vary are already set via set_default_headers.
    auto origin = req.get_header_value("Origin");
    for (const auto& allowed : cors_origins_) {
        if (origin == allowed) {
            res.set_header("Access-Control-Allow-Origin", origin);
            return;
        }
    }
}

void RestRouter::bind(http::Server& server) {
    // Static CORS headers are set via set_default_headers so they appear on
    // every response, including httplib's built-in 404 for unmatched routes.
    //
    // For wildcard or single-origin configs, all CORS headers are static.
    // For multi-origin, Allow-Origin varies per request and is set in each
    // handler via apply_cors_origin(); the remaining headers are static.
    if (cors_wildcard_) {
        server.set_default_headers({
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
        });
    }
    else if (cors_origins_.size() == 1) {
        server.set_default_headers({
            {"Access-Control-Allow-Origin", cors_origins_[0]},
            {"Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
        });
    }
    else if (!cors_origins_.empty()) {
        server.set_default_headers({
            {"Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
            {"Vary", "Origin"},
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
        if (cors_origins_.size() > 1) {
            server.Options(pattern, [this](const http::Request& req, http::Response& res) {
                apply_cors_origin(req, res);
                res.status = 204;
            });
        }
        else {
            server.Options(pattern, [](const http::Request&, http::Response& res) { res.status = 204; });
        }
    }
}

void RestRouter::dispatch(RestEndpoint& endpoint, const http::Request& req, http::Response& res) {
    // Static CORS headers are set via set_default_headers in bind().
    // Multi-origin mode needs per-request Allow-Origin.
    if (cors_origins_.size() > 1)
        apply_cors_origin(req, res);

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
