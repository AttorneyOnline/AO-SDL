#pragma once

#include "event/Event.h"

#include <string>

/// An event to be delivered to Server-Sent Events (SSE) clients.
/// Published to EventManager by game logic (e.g., NXServer broadcast callbacks).
/// Consumed by the HTTP server's poll loop and written to open SSE connections.
struct SSEEvent : public Event {
    std::string event;        ///< SSE event type (e.g., "ic", "ooc", "music", "char_select")
    std::string data;         ///< JSON payload
    std::string area;         ///< Target area (empty = broadcast to all SSE clients)
    std::string target_token; ///< Target session token (empty = broadcast). When set,
                              ///< event is delivered only to the SSE connection whose
                              ///< session_token matches.
};
