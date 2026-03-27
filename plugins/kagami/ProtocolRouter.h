/**
 * @file ProtocolRouter.h
 * @brief Routes WebSocket clients to the correct protocol backend
 *        based on the negotiated subprotocol.
 *
 * "ao2" or no subprotocol → AOServer (legacy)
 * Anything else            → NXServer
 */
#pragma once

#include "net/ao/AOServer.h"
#include "net/nx/NXServer.h"
#include "utils/Log.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

class ProtocolRouter {
  public:
    using SubprotocolFunc = std::function<std::string(uint64_t client_id)>;

    ProtocolRouter(AOServer& ao, NXServer& nx) : ao_(ao), nx_(nx) {
    }

    /// Set the function that resolves a client's negotiated subprotocol.
    /// Typically wired to WebSocketServer::get_client_subprotocol().
    void set_subprotocol_func(SubprotocolFunc func) {
        subproto_func_ = std::move(func);
    }

    void on_client_connected(uint64_t client_id) {
        bool legacy = is_legacy(client_id);
        routing_[client_id] = legacy;
        if (legacy)
            ao_.on_client_connected(client_id);
        else
            nx_.create_session(client_id);
    }

    void on_client_disconnected(uint64_t client_id) {
        auto it = routing_.find(client_id);
        if (it == routing_.end())
            return;
        if (it->second)
            ao_.on_client_disconnected(client_id);
        else
            nx_.destroy_session(client_id);
        routing_.erase(it);
    }

    void on_client_message(uint64_t client_id, const std::string& data) {
        auto it = routing_.find(client_id);
        if (it == routing_.end())
            return;
        if (it->second)
            ao_.on_client_message(client_id, data);
        else {
            // NX messages are handled via REST; WS is broadcast-only.
            Log::log_print(WARNING, "NX: unexpected WS data from client %llu (NX uses REST)",
                           (unsigned long long)client_id);
        }
    }

    AOServer& ao() {
        return ao_;
    }
    NXServer& nx() {
        return nx_;
    }

  private:
    bool is_legacy(uint64_t client_id) const {
        if (!subproto_func_)
            return true;
        auto proto = subproto_func_(client_id);
        return proto.empty() || proto == "ao2";
    }

    AOServer& ao_;
    NXServer& nx_;
    std::unordered_map<uint64_t, bool> routing_; ///< client_id → true if legacy
    SubprotocolFunc subproto_func_;
};
