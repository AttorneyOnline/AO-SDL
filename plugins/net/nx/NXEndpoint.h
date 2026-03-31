#pragma once

#include "NXServer.h"
#include "net/RestEndpoint.h"

#include <atomic>
#include <cassert>

/// Base class for AONX REST endpoint handlers.
/// Provides access to NXServer and GameRoom via an atomic static pointer
/// that is set once at startup before any handler is called.
class NXEndpoint : public RestEndpoint {
  public:
    static void set_server(NXServer* server) {
        server_.store(server, std::memory_order_release);
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

  private:
    inline static std::atomic<NXServer*> server_ = nullptr;
};

/// Call once at startup to ensure all EndpointRegistrar statics are
/// initialized. Prevents the linker from stripping the translation
/// units when linking as a static library.
void nx_register_endpoints();
