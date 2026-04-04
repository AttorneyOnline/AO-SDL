#pragma once

#include "net/RestEndpoint.h"

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
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
    void set_cors_origins(std::vector<std::string> origins);

    /// Takes ownership of an endpoint.
    void register_endpoint(std::unique_ptr<RestEndpoint> endpoint);

    /// Bind all registered endpoints to the http server.
    /// Call once after all endpoints have been registered.
    void bind(http::Server& server);

    /// Execute a callable under an exclusive dispatch lock.
    /// Use for operations that mutate game state and must be serialized
    /// with endpoint handlers (e.g., session expiry sweeps, AO protocol).
    template <typename F>
    void with_lock(F&& func) {
        std::unique_lock lock(dispatch_mutex_);
        func();
    }

    /// Execute a callable under a shared (reader) dispatch lock.
    /// Use for operations that only read game state and can run concurrently
    /// with other readers.
    template <typename F>
    void with_shared_lock(F&& func) {
        std::shared_lock lock(dispatch_mutex_);
        func();
    }

    /// Try to execute a callable under a shared lock. Returns false without
    /// calling func if the lock cannot be acquired immediately (e.g., an
    /// exclusive writer is active). Use for best-effort reads where stale
    /// data is acceptable.
    template <typename F>
    bool try_shared_lock(F&& func) {
        std::shared_lock lock(dispatch_mutex_, std::try_to_lock);
        if (!lock.owns_lock())
            return false;
        func();
        return true;
    }

  private:
    void dispatch(RestEndpoint& endpoint, const http::Request& req, http::Response& res);
    void apply_cors_origin(const http::Request& req, http::Response& res) const;
    bool cors_enabled() const;

    AuthFunc auth_func_;
    std::vector<std::string> cors_origins_;
    bool cors_wildcard_ = false;
    std::vector<std::unique_ptr<RestEndpoint>> endpoints_;
    std::shared_mutex dispatch_mutex_; ///< Serializes handler access to game state.
};
