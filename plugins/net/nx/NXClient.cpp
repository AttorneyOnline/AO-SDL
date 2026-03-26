#include "NXClient.h"

#include "utils/Log.h"

NXClient::NXClient() = default;

void NXClient::on_connect() {
    state_ = NXConnectionState::AUTHENTICATING;
    Log::log_print(INFO, "NX: WebSocket connected, beginning handshake");

    // TODO: Send session token to authenticate the WebSocket leg.
    // The REST leg handles initial auth; the WS just proves identity.
}

void NXClient::on_message(const std::string& msg) {
    auto message = NXMessage::deserialize(msg);
    if (!message) {
        Log::log_print(WARNING, "NX: failed to parse message");
        return;
    }

    dispatch(*message);
}

void NXClient::on_disconnect() {
    state_ = NXConnectionState::DISCONNECTED;
    session_token_.clear();
    Log::log_print(INFO, "NX: disconnected");
}

std::vector<std::string> NXClient::flush_outgoing() {
    std::vector<std::string> out;
    out.swap(outgoing_);
    return out;
}

void NXClient::dispatch(const NXMessage& msg) {
    // TODO: Route by message type to handler functions.
    // Each handler posts the appropriate game event.
    //
    // Example (pseudocode):
    //   switch (msg.type()) {
    //   case "ic_message":  handle_ic_message(msg); break;
    //   case "ooc_message": handle_ooc_message(msg); break;
    //   case "background":  handle_background(msg); break;
    //   case "music":       handle_music(msg); break;
    //   ...
    //   }

    Log::log_print(DEBUG, "NX: received '%s' (v%d)", msg.type().c_str(), msg.schema_version());
}
