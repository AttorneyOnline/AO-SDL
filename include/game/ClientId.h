#pragma once

#include <cstdint>
#include <string>

/// Format a client_id for logging.
/// High bit set → NX session (REST), unset → AO2 session (WebSocket).
inline std::string format_client_id(uint64_t client_id) {
    constexpr uint64_t NX_BIT = 0x8000'0000'0000'0000ULL;
    if (client_id & NX_BIT) {
        return "NX" + std::to_string(client_id & ~NX_BIT);
    }
    return "AO" + std::to_string(client_id);
}
