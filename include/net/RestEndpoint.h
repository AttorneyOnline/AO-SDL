#pragma once

#include "net/RestRequest.h"
#include "net/RestResponse.h"

#include <string>

/// Per-endpoint CORS policy. Controls how the Access-Control-Allow-Origin
/// header is chosen for this route, independent of the router's global
/// origin list. Layering CORS per endpoint lets the server advertise a
/// permissive policy for discovery endpoints (server info) while forcing
/// same-origin for admin/moderation endpoints even when the router is
/// configured with a wildcard.
enum class CorsPolicy {
    /// Inherit the router's configured origin set. This is the right
    /// choice for the normal game API.
    Default,
    /// Always respond with Access-Control-Allow-Origin: *. Use for
    /// unauthenticated discovery endpoints (MOTD, server info, health)
    /// that any page should be able to query from a browser.
    Public,
    /// Never emit Access-Control-Allow-Origin. Browsers block cross-origin
    /// requests to this route regardless of router config. Use for
    /// privileged endpoints (admin, moderation) where credentialed
    /// cross-origin use would be a CSRF/XS-request vector.
    Restricted,
};

/// Abstract base for REST endpoint handlers.
/// Concrete endpoints override this to declare their route and implement
/// request handling. Parallels AOPacket in the AO2 protocol layer.
class RestEndpoint {
  public:
    virtual ~RestEndpoint() = default;

    virtual const std::string& method() const = 0;
    virtual const std::string& path_pattern() const = 0;
    virtual bool requires_auth() const = 0;

    /// If true, response bodies are redacted in logs to avoid leaking secrets.
    virtual bool sensitive() const {
        return false;
    }

    /// If true, the handler only reads game state and can run concurrently
    /// with other read-only handlers under a shared lock.
    virtual bool readonly() const {
        return false;
    }

    /// If true, the handler manages its own synchronization and runs with
    /// NO dispatch lock held. Use for endpoints backed by lock-free data
    /// structures (e.g., HAMT-based session create/delete).
    virtual bool lock_free() const {
        return false;
    }

    /// CORS tier for this endpoint. Defaults to the router's config.
    virtual CorsPolicy cors_policy() const {
        return CorsPolicy::Default;
    }

    virtual RestResponse handle(const RestRequest& req) = 0;
};
