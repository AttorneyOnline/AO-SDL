#include "net/RestRouter.h"

#include "game/ServerSession.h"
#include "metrics/MetricsRegistry.h"
#include "utils/Log.h"

#include "net/Http.h"

#include <chrono>
#include <cstring>

// -- Metrics (file-scope statics — self-register at program startup) ----------

static auto& http_requests_ = metrics::MetricsRegistry::instance().counter(
    "kagami_http_requests_total", "Total HTTP requests handled", {"method", "endpoint", "status"});
static auto& http_response_bytes_ = metrics::MetricsRegistry::instance().counter(
    "kagami_http_response_bytes_total", "Total HTTP response bytes sent", {"method", "endpoint"});
static auto& dispatch_lock_acquisitions_ = metrics::MetricsRegistry::instance().counter(
    "kagami_dispatch_lock_acquisitions_total", "Dispatch mutex lock acquisitions");
static auto& http_errors_ =
    metrics::MetricsRegistry::instance().counter("kagami_http_errors_total", "Protocol-level HTTP errors", {"type"});
static auto& dispatch_lock_wait_ = metrics::MetricsRegistry::instance().counter(
    "kagami_dispatch_lock_wait_nanoseconds_total", "Cumulative dispatch mutex wait time in nanoseconds");
static auto& dispatch_lock_hold_ = metrics::MetricsRegistry::instance().counter(
    "kagami_dispatch_lock_hold_nanoseconds_total", "Cumulative dispatch mutex hold time in nanoseconds", {"endpoint"});
static auto& http_request_bytes_ = metrics::MetricsRegistry::instance().counter(
    "kagami_http_request_bytes_total", "Total HTTP request body bytes received", {"method", "endpoint"});

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
            {"Access-Control-Allow-Headers", "*"},
        });
    }
    else if (cors_origins_.size() == 1) {
        server.set_default_headers({
            {"Access-Control-Allow-Origin", cors_origins_[0]},
            {"Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS"},
            {"Access-Control-Allow-Headers", "*"},
        });
    }
    else if (!cors_origins_.empty()) {
        server.set_default_headers({
            {"Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS"},
            {"Access-Control-Allow-Headers", "*"},
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
    Log::log_print(VERBOSE, "REST: >> %s %s", req.method.c_str(), req.path.c_str());

    // Static CORS headers are set via set_default_headers in bind().
    // Multi-origin mode needs per-request Allow-Origin.
    if (cors_origins_.size() > 1)
        apply_cors_origin(req, res);

    try {
        // Build RestRequest from http::Request (no lock needed — pure parsing)
        RestRequest rest_req;
        rest_req.method = req.method;
        rest_req.path = req.path;
        rest_req.remote_addr = req.remote_addr;

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
                Log::log_print(VERBOSE, "REST: << 400 %s %s (malformed JSON)", req.method.c_str(), req.path.c_str());
                http_errors_.labels({"malformed_json"}).inc();
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

        // Lock strategy depends on endpoint type:
        //   lock_free:  no dispatch lock — endpoint manages own synchronization
        //   readonly:   shared lock — concurrent with other readers
        //   mutating:   exclusive lock — serialized with all other endpoints
        RestResponse rest_res;
        {
            auto lock_start = std::chrono::steady_clock::now();

            std::unique_lock<std::shared_mutex> exclusive_lock(dispatch_mutex_, std::defer_lock);
            std::shared_lock<std::shared_mutex> shared_lock(dispatch_mutex_, std::defer_lock);
            if (!endpoint.lock_free()) {
                if (endpoint.readonly())
                    shared_lock.lock();
                else
                    exclusive_lock.lock();
            }

            auto lock_acquired = std::chrono::steady_clock::now();
            dispatch_lock_acquisitions_.get().inc();
            auto wait_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(lock_acquired - lock_start).count();
            if (wait_ns > 0)
                dispatch_lock_wait_.get().inc(static_cast<uint64_t>(wait_ns));

            if (endpoint.requires_auth()) {
                if (rest_req.bearer_token.empty()) {
                    Log::log_print(VERBOSE, "REST: << 401 %s %s (no token)", req.method.c_str(), req.path.c_str());
                    http_errors_.labels({"missing_token"}).inc();
                    res.status = 401;
                    res.set_content(R"({"reason":"Missing session token"})", "application/json");
                    return;
                }
                if (!auth_func_) {
                    res.status = 500;
                    res.set_content(R"({"reason":"Server authentication is not configured"})", "application/json");
                    return;
                }

                // Auth lookup is lock-free — reads HAMT snapshot via find()
                rest_req.session = auth_func_(rest_req.bearer_token);
                if (!rest_req.session) {
                    Log::log_print(VERBOSE, "REST: << 401 %s %s (bad token)", req.method.c_str(), req.path.c_str());
                    http_errors_.labels({"invalid_token"}).inc();
                    res.status = 401;
                    res.set_content(R"({"reason":"Invalid or expired session token"})", "application/json");
                    return;
                }
                rest_req.session->touch();
            }

            rest_res = endpoint.handle(rest_req);

            auto hold_ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - lock_acquired)
                    .count();
            dispatch_lock_hold_.labels({endpoint.path_pattern()}).inc(static_cast<uint64_t>(hold_ns));
        }

        // Track request body size (outside lock)
        if (rest_req.body)
            http_request_bytes_.labels({req.method, endpoint.path_pattern()}).inc(req.body.size());

        // Write response (outside lock — no game state access)
        res.status = rest_res.status;
        if (rest_res.status == 204 || rest_res.body.is_null()) {
            Log::log_print(VERBOSE, "REST: << %d %s %s", rest_res.status, req.method.c_str(), req.path.c_str());
            // Per-session traffic counters (request only, no response body)
            if (rest_req.session) {
                rest_req.session->bytes_received.fetch_add(req.body.size(), std::memory_order_relaxed);
                rest_req.session->packets_received.fetch_add(1, std::memory_order_relaxed);
                rest_req.session->packets_sent.fetch_add(1, std::memory_order_relaxed);
            }
        }
        else {
            auto body = rest_res.body.dump();
            if (endpoint.sensitive())
                Log::log_print(VERBOSE, "REST: << %d %s %s [REDACTED]", rest_res.status, req.method.c_str(),
                               req.path.c_str());
            else
                Log::log_print(VERBOSE, "REST: << %d %s %s %s", rest_res.status, req.method.c_str(), req.path.c_str(),
                               body.c_str());
            http_response_bytes_.labels({req.method, endpoint.path_pattern()}).inc(body.size());
            // Per-session traffic counters
            if (rest_req.session) {
                rest_req.session->bytes_received.fetch_add(req.body.size(), std::memory_order_relaxed);
                rest_req.session->packets_received.fetch_add(1, std::memory_order_relaxed);
                rest_req.session->bytes_sent.fetch_add(body.size(), std::memory_order_relaxed);
                rest_req.session->packets_sent.fetch_add(1, std::memory_order_relaxed);
            }
            res.set_content(std::move(body), rest_res.content_type);
        }

        http_requests_.labels({req.method, endpoint.path_pattern(), std::to_string(rest_res.status)}).inc();
    }
    catch (const std::exception& e) {
        Log::log_print(ERR, "REST: exception in %s %s: %s", endpoint.method().c_str(), endpoint.path_pattern().c_str(),
                       e.what());
        res.status = 500;
        auto err_body = R"({"reason":"An internal server error occurred"})";
        res.set_content(err_body, "application/json");
        http_requests_.labels({req.method, endpoint.path_pattern(), "500"}).inc();
        http_response_bytes_.labels({req.method, endpoint.path_pattern()}).inc(std::strlen(err_body));
    }
}
