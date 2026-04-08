#pragma once

#include "NXServer.h"
#include "net/RateLimiter.h"
#include "net/RestEndpoint.h"

#include <atomic>
#include <cassert>

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
