/**
 * @file ProtocolRouter.h
 * @brief Routes WebSocket clients to the AO2 protocol backend.
 *
 * Only AO2 uses WebSocket for bidirectional communication.
 * AONX clients use REST + SSE (no WebSocket).
 */
#pragma once

#include "net/ao/AOServer.h"

#include <cstdint>
#include <string>

class ProtocolRouter {
  public:
    explicit ProtocolRouter(AOServer& ao) : ao_(ao) {
    }

    void on_client_connected(uint64_t client_id) {
        ao_.on_client_connected(client_id);
    }

    void on_client_disconnected(uint64_t client_id) {
        ao_.on_client_disconnected(client_id);
    }

    void on_client_message(uint64_t client_id, const std::string& data) {
        ao_.on_client_message(client_id, data);
    }

  private:
    AOServer& ao_;
};
