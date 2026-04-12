#pragma once

#include "NXServer.h"
#include "moderation/ModerationTypes.h"
#include "net/RateLimiter.h"
#include "net/RestEndpoint.h"

#include <atomic>
#include <cassert>
#include <optional>

class ServerSession;

/// Base class for AONX REST endpoint handlers.
/// Provides access to NXServer, GameRoom, and RateLimiter via atomic static
/// pointers set once at startup before any handler is called.
class NXEndpoint : public RestEndpoint {
  public:
    static void set_server(NXServer* server) {
        server_.store(server, std::memory_order_release);
    }
    static void set_rate_limiter(net::RateLimiter* limiter) {
        rate_limiter_.store(limiter, std::memory_order_release);
    }

  public:
    /// Outcome of applying a content moderation verdict to an NX
    /// session: a value tells the caller "stop processing the
    /// request and return this RestResponse"; std::nullopt means
    /// "continue, possibly with a mutated message".
    enum class ContentVerdictPass : int {
        Pass,   ///< Verdict was NONE or LOG — continue normally.
        Censor, ///< CENSOR — caller should swap message to "[filtered]" and continue.
    };
    struct ContentVerdictResult {
        std::optional<RestResponse> early_return; ///< Set when caller must return immediately.
        ContentVerdictPass pass_kind = ContentVerdictPass::Pass;
    };

    /// Apply a moderation verdict to an NX session: dispatch the
    /// appropriate side effect (BanManager add for BAN/PERMA_BAN,
    /// session destruction for KICK/BAN/PERMA_BAN), and return what
    /// the caller should do with the request.
    ///
    /// Mirrors `apply_content_verdict()` in the AO2 ServerPacketBehavior
    /// anonymous namespace, but adapted for the REST shape: returns a
    /// RestResponse to send back to the client when the request must
    /// be rejected, and uses NXServer::destroy_session() instead of
    /// the AO2 deferred-close path.
    ///
    /// Pre-fix this layer was a no-op for KICK/BAN/PERMA_BAN — just
    /// returned 403 without invalidating the session token or adding
    /// to the ban table. An attacker could keep POSTing IC/OOC after
    /// being "banned" because the heat decayed back to zero between
    /// requests. This helper closes that gap and is the central place
    /// to fix any future related bug.
    ///
    /// Public so unit tests can drive it directly without round-
    /// tripping through the HTTP layer.
    static ContentVerdictResult
    apply_content_verdict(ServerSession& session, const moderation::ModerationVerdict& verdict, const char* channel);

  protected:
    static NXServer& server() {
        auto* s = server_.load(std::memory_order_acquire);
        assert(s && "NXEndpoint::set_server() must be called before any endpoint handles a request");
        return *s;
    }
    static GameRoom& room() {
        return server().room();
    }
    static net::RateLimiter* rate_limiter() {
        return rate_limiter_.load(std::memory_order_acquire);
    }

  private:
    inline static std::atomic<NXServer*> server_ = nullptr;
    inline static std::atomic<net::RateLimiter*> rate_limiter_ = nullptr;
};

/// Call once at startup to ensure all EndpointRegistrar statics are
/// initialized. Prevents the linker from stripping the translation
/// units when linking as a static library.
void nx_register_endpoints();
