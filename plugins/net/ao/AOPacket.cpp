#include "AOPacket.h"

#include "AOClient.h"
#include "AOServer.h"
#include "PacketFactory.h"
#include "utils/Log.h"

#include <format>

AOPacket::AOPacket() : valid(false) {
}

AOPacket::AOPacket(std::string header, std::vector<std::string> fields) : valid(true), header(header), fields(fields) {
}

std::unique_ptr<AOPacket> AOPacket::deserialize(const std::string& serialized) {
    // Step 1: Validate the delimiter
    const std::string DELIMITER = "#%";
    if (serialized.size() < DELIMITER.size() || serialized.substr(serialized.size() - DELIMITER.size()) != DELIMITER) {
        throw std::invalid_argument("Invalid packet format: missing delimiter '#%'.");
    }

    // Step 2: Remove the delimiter
    std::string content = serialized.substr(0, serialized.size() - DELIMITER.size());

    // Step 3: Split the content by '#'
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t pos = content.find('#');

    while (pos != std::string::npos) {
        tokens.emplace_back(content.substr(start, pos - start));
        start = pos + 1;
        pos = content.find('#', start);
    }

    // Add the last token
    tokens.emplace_back(content.substr(start));

    if (tokens.empty()) {
        throw std::invalid_argument("Empty packet.");
    }

    // Step 4: Extract header and fields
    std::string header = tokens[0];
    std::vector<std::string> fields(tokens.begin() + 1, tokens.end());

    return PacketFactory::instance().create_packet(header, fields);
}

std::string AOPacket::serialize() const {
    std::string packet = header;

    for (auto field : fields) {
        packet = std::format("{}#{}", packet, field);
    }

    packet = std::format("{}#%", packet);
    return packet;
}

bool AOPacket::is_valid() {
    return valid;
}

void AOPacket::handle(AOClient& cli) {
    Log::log_print(DEBUG, "Unhandled client packet %s", header.c_str());
}

void AOPacket::handle_server(AOServer& /*server*/, ServerSession& /*session*/) {
    Log::log_print(DEBUG, "Unhandled server packet %s", header.c_str());
}

// ---------------------------------------------------------------------------
// ao_packet::log_bad_packet
// ---------------------------------------------------------------------------

namespace ao_packet {

// Cap the logged packet preview. MS can legitimately reach ~500 bytes, but a
// hostile client could send arbitrarily long garbage; we refuse to pay the log
// bandwidth for them. 256 bytes covers every well-formed packet type comfortably.
static constexpr size_t MAX_LOG_PREVIEW = 256;

// Strip non-printable bytes so log sinks (journald, CloudWatch, terminals)
// can't be confused by embedded control chars, ANSI escapes, or binary blobs
// in hostile payloads.
static std::string redact_for_log(const std::string& wire) {
    std::string out;
    const size_t limit = std::min(wire.size(), MAX_LOG_PREVIEW);
    out.reserve(limit + 3);
    for (size_t i = 0; i < limit; ++i) {
        const unsigned char c = static_cast<unsigned char>(wire[i]);
        out += (c >= 32 && c < 127) ? static_cast<char>(c) : '?';
    }
    if (wire.size() > MAX_LOG_PREVIEW)
        out += "...";
    return out;
}

void log_bad_packet(std::string_view peer_label, const std::string& wire, std::string_view reason) {
    // log_print is printf-style; copy the views into std::string so we can
    // pass stable C strings without worrying about string_view's non-null-
    // terminated contract.
    const std::string peer(peer_label);
    const std::string reason_str(reason);
    const std::string preview = redact_for_log(wire);
    Log::log_print(WARNING, "AO: malformed packet from [%s]: %s — wire=%s", peer.c_str(), reason_str.c_str(),
                   preview.c_str());
}

} // namespace ao_packet
