#pragma once

#include "net/RestEndpoint.h"

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace http {
class Server;
struct Request;
struct Response;
} // namespace http

struct ServerSession;

/// Routes HTTP requests to registered RestEndpoint handlers.
/// Handles JSON parsing, bearer-token auth, error formatting, and
/// thread-safe dispatch (http runs handlers on its own thread pool).
class RestRouter {
  public:
    /// Given a bearer token, return the owning session or nullptr.
    using AuthFunc = std::function<ServerSession*(const std::string& token)>;

    void set_auth_func(AuthFunc func);
    void set_cors_origin(const std::string& origin);

    /// Takes ownership of an endpoint.
    void register_endpoint(std::unique_ptr<RestEndpoint> endpoint);

    /// Bind all registered endpoints to the http server.
    /// Call once after all endpoints have been registered.
    void bind(http::Server& server);

    /// Execute a callable under the dispatch mutex.
    /// Use for operations that must be serialized with endpoint handlers
    /// (e.g., session expiry sweeps).
    template <typename F>
    void with_lock(F&& func) {
        std::lock_guard lock(dispatch_mutex_);
        func();
    }

  private:
    void dispatch(RestEndpoint& endpoint, const http::Request& req, http::Response& res);
    void set_cors(http::Response& res);

    AuthFunc auth_func_;
    std::string cors_origin_;
    std::vector<std::unique_ptr<RestEndpoint>> endpoints_;
    std::mutex dispatch_mutex_; ///< Serializes handler access to game state.
};
