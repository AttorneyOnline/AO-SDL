#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class AOClient;
class AOServer;
struct ServerSession;

class PacketFormatException : public std::invalid_argument {
  public:
    explicit PacketFormatException(const std::string& message) : std::invalid_argument(message) {
    }
};

class ProtocolStateException : public std::runtime_error {
  public:
    explicit ProtocolStateException(const std::string& message) : std::runtime_error(message) {
    }
};

class AOPacket {
  public:
    AOPacket();
    AOPacket(std::string header, std::vector<std::string> fields);

    std::string serialize() const;
    static std::unique_ptr<AOPacket> deserialize(const std::string& serialized);
    bool is_valid();

    /// Client-side handler: process a packet received from the server.
    virtual void handle(AOClient& cli);

    /// Server-side handler: process a packet received from a client.
    virtual void handle_server(AOServer& server, ServerSession& session);

    static constexpr const char* DELIMITER = "#%";

    /// Public read-only access to the packet type (e.g. "MS", "CT", "CC").
    const std::string& get_header() const {
        return header;
    }

  protected:
    bool valid;

    std::string header;
    std::vector<std::string> fields;
};

// ---------------------------------------------------------------------------
// Safe deserialize + dispatch boundary
// ---------------------------------------------------------------------------
//
// Both `AOClient` (processing server→client frames) and `AOServer` (processing
// client→server frames) must cross this boundary before any typed packet
// handler runs. Packet constructors can throw (`std::stoi` on a non-numeric
// field, `PacketFormatException` from validated parsers) and handlers can
// throw (`ProtocolStateException`, lookup failures, downstream bugs) — a
// single uncaught exception on a worker thread calls `std::terminate()` and
// kills the process.
//
// Callers should never invoke `AOPacket::deserialize` + a handler in sequence
// directly. Always go through `ao_packet::safe_dispatch`, which wraps *both*
// steps in a single try/catch, logs failures with a peer label and a
// redacted wire preview, and returns a categorized `DispatchResult` so the
// caller can update metrics without re-implementing exception handling.
namespace ao_packet {

enum class DispatchResult {
    Ok,            ///< Packet parsed and handler returned normally.
    ParseFailed,   ///< Exception thrown during `AOPacket::deserialize`.
    HandlerFailed, ///< Handler threw after a successful parse.
    InvalidPacket, ///< Parsed to an `AOPacket` whose `is_valid()` is false.
};

/// Log a malformed packet at WARNING. Printable ASCII is forwarded
/// verbatim; other bytes are replaced with '?' and the preview is
/// truncated so adversarial payloads cannot scramble the terminal or
/// inflate log volume.
///
/// Out-of-line so the template below does not pull in `utils/Log.h`
/// for every translation unit that includes `AOPacket.h`.
void log_bad_packet(std::string_view peer_label, const std::string& wire, std::string_view reason);

/// Deserialize `wire` and invoke `dispatch(AOPacket&)` on success.
///
/// All exceptions — from deserialization *or* from the dispatch callback —
/// are caught, logged via `log_bad_packet`, and suppressed. The framing
/// loop that called this function can safely continue with the next
/// packet in the buffer.
///
/// @tparam DispatchFn Callable with signature `void(AOPacket&)`.
/// @param wire        Fully framed packet content (including `#%` delimiter).
/// @param peer_label  Short human label for log context — caller's choice
///                    of granularity (e.g. `"AOClient"`, `"client=42 10.0.0.1"`).
/// @param dispatch    Invoked with the parsed packet if deserialization
///                    succeeded and the packet reports `is_valid()`.
///
/// @returns `DispatchResult` indicating the outcome. Callers typically
/// use this to increment per-category metrics counters.
template <typename DispatchFn>
DispatchResult safe_dispatch(const std::string& wire, std::string_view peer_label, DispatchFn&& dispatch) {
    std::unique_ptr<AOPacket> pkt;
    try {
        pkt = AOPacket::deserialize(wire);
    }
    catch (const std::exception& e) {
        log_bad_packet(peer_label, wire, e.what());
        return DispatchResult::ParseFailed;
    }
    if (!pkt || !pkt->is_valid()) {
        log_bad_packet(peer_label, wire, "invalid packet (is_valid() == false)");
        return DispatchResult::InvalidPacket;
    }
    try {
        dispatch(*pkt);
        return DispatchResult::Ok;
    }
    catch (const std::exception& e) {
        log_bad_packet(peer_label, wire, e.what());
        return DispatchResult::HandlerFailed;
    }
}

} // namespace ao_packet
