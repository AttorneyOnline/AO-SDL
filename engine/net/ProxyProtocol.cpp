#include "net/ProxyProtocol.h"

#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace net {

// PROXY protocol v2 signature (12 bytes)
static const uint8_t PP2_SIGNATURE[] = {0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A};
static constexpr size_t PP2_SIGNATURE_LEN = 12;
static constexpr size_t PP2_HEADER_LEN = 16; // signature(12) + ver_cmd(1) + fam(1) + len(2)

// -- v1 parser ----------------------------------------------------------------

static ProxyProtocolResult parse_v1(const uint8_t* data, size_t len) {
    // v1 format: "PROXY TCP4 srcip dstip srcport dstport\r\n"
    // or:        "PROXY TCP6 srcip dstip srcport dstport\r\n"
    // or:        "PROXY UNKNOWN\r\n"
    // Max line length: 107 bytes (per spec)

    // Find \r\n
    const size_t max_line = (len < 108) ? len : 108;
    size_t line_end = 0;
    bool found = false;
    for (size_t i = 0; i + 1 < max_line; ++i) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            line_end = i;
            found = true;
            break;
        }
    }

    if (!found) {
        // Could be truncated if we haven't seen 108 bytes yet
        ProxyProtocolResult r;
        r.need_more_data = (len < 108);
        return r;
    }

    std::string line(reinterpret_cast<const char*>(data), line_end);
    ProxyProtocolResult result;
    result.header_length = line_end + 2; // include \r\n

    // Parse "PROXY <protocol> <src_ip> <dst_ip> <src_port> <dst_port>"
    // Skip "PROXY " prefix (already verified by caller)
    if (line.size() < 7)
        return result; // too short

    std::string rest = line.substr(6); // after "PROXY "

    // Handle UNKNOWN protocol
    if (rest.starts_with("UNKNOWN")) {
        result.valid = true;
        // No address info — client_addr stays empty
        return result;
    }

    // Parse "TCP4 src dst sport dport" or "TCP6 src dst sport dport"
    char proto[8] = {};
    char src_ip[64] = {};
    char dst_ip[64] = {};
    unsigned src_port = 0, dst_port = 0;

    if (sscanf(rest.c_str(), "%7s %63s %63s %u %u", proto, src_ip, dst_ip, &src_port, &dst_port) != 5)
        return result;

    if (std::strcmp(proto, "TCP4") != 0 && std::strcmp(proto, "TCP6") != 0)
        return result;

    // Validate the parsed IP by round-tripping through inet_pton
    if (std::strcmp(proto, "TCP4") == 0) {
        struct in_addr addr{};
        if (inet_pton(AF_INET, src_ip, &addr) != 1)
            return result;
    }
    else {
        struct in6_addr addr{};
        if (inet_pton(AF_INET6, src_ip, &addr) != 1)
            return result;
    }

    result.valid = true;
    result.client_addr = src_ip;
    result.client_port = static_cast<uint16_t>(src_port);
    result.server_addr = dst_ip;
    result.server_port = static_cast<uint16_t>(dst_port);

    return result;
}

// -- v2 parser ----------------------------------------------------------------

static ProxyProtocolResult parse_v2(const uint8_t* data, size_t len) {
    if (len < PP2_HEADER_LEN) {
        ProxyProtocolResult r;
        r.need_more_data = true;
        return r;
    }

    // Byte 12: version (upper 4 bits) and command (lower 4 bits)
    uint8_t ver_cmd = data[12];
    uint8_t version = (ver_cmd >> 4) & 0x0F;
    uint8_t command = ver_cmd & 0x0F;

    if (version != 2)
        return {}; // Only v2 supported

    // Byte 13: address family (upper 4 bits) and transport (lower 4 bits)
    uint8_t fam_trans = data[13];
    uint8_t family = (fam_trans >> 4) & 0x0F;
    // uint8_t transport = fam_trans & 0x0F; // 1=STREAM, 2=DGRAM

    // Bytes 14-15: length of address data (big-endian)
    uint16_t addr_len = static_cast<uint16_t>((data[14] << 8) | data[15]);

    size_t total_len = PP2_HEADER_LEN + addr_len;
    if (len < total_len) {
        ProxyProtocolResult r;
        r.need_more_data = true;
        return r;
    }

    ProxyProtocolResult result;
    result.header_length = total_len;

    // Command 0 = LOCAL (health check), 1 = PROXY
    if (command == 0) {
        result.valid = true;
        // LOCAL connection — no address override
        return result;
    }

    if (command != 1)
        return result; // Unknown command

    const uint8_t* addr_data = data + PP2_HEADER_LEN;

    if (family == 1 && addr_len >= 12) {
        // AF_INET: 4 bytes src + 4 bytes dst + 2 bytes src_port + 2 bytes dst_port
        char buf[INET_ADDRSTRLEN];
        struct in_addr src_addr{};
        std::memcpy(&src_addr, addr_data, 4);
        inet_ntop(AF_INET, &src_addr, buf, sizeof(buf));
        result.client_addr = buf;

        struct in_addr dst_addr{};
        std::memcpy(&dst_addr, addr_data + 4, 4);
        inet_ntop(AF_INET, &dst_addr, buf, sizeof(buf));
        result.server_addr = buf;

        result.client_port = static_cast<uint16_t>((addr_data[8] << 8) | addr_data[9]);
        result.server_port = static_cast<uint16_t>((addr_data[10] << 8) | addr_data[11]);
        result.valid = true;
    }
    else if (family == 2 && addr_len >= 36) {
        // AF_INET6: 16 bytes src + 16 bytes dst + 2 bytes src_port + 2 bytes dst_port
        char buf[INET6_ADDRSTRLEN];
        struct in6_addr src_addr{};
        std::memcpy(&src_addr, addr_data, 16);
        inet_ntop(AF_INET6, &src_addr, buf, sizeof(buf));
        result.client_addr = buf;

        struct in6_addr dst_addr{};
        std::memcpy(&dst_addr, addr_data + 16, 16);
        inet_ntop(AF_INET6, &dst_addr, buf, sizeof(buf));
        result.server_addr = buf;

        result.client_port = static_cast<uint16_t>((addr_data[32] << 8) | addr_data[33]);
        result.server_port = static_cast<uint16_t>((addr_data[34] << 8) | addr_data[35]);
        result.valid = true;
    }
    else if (family == 0) {
        // AF_UNSPEC — no address
        result.valid = true;
    }

    return result;
}

// -- Entry point --------------------------------------------------------------

ProxyProtocolResult parse_proxy_protocol(const uint8_t* data, size_t len) {
    if (len == 0) {
        ProxyProtocolResult r;
        r.need_more_data = true;
        return r;
    }

    // Check for v1 ("PROXY " ASCII prefix)
    if (len >= 6 && std::memcmp(data, "PROXY ", 6) == 0)
        return parse_v1(data, len);

    // Check for v2 (binary signature)
    if (len >= PP2_SIGNATURE_LEN && std::memcmp(data, PP2_SIGNATURE, PP2_SIGNATURE_LEN) == 0)
        return parse_v2(data, len);

    // If we have less than the signature length, could still be either
    if (len < PP2_SIGNATURE_LEN) {
        // Check if what we have so far is a prefix of either signature
        if (len < 6 && std::memcmp(data, "PROXY ", len) == 0) {
            ProxyProtocolResult r;
            r.need_more_data = true;
            return r;
        }
        if (std::memcmp(data, PP2_SIGNATURE, len) == 0) {
            ProxyProtocolResult r;
            r.need_more_data = true;
            return r;
        }
    }

    // Not a PROXY protocol header — this is a direct connection
    return {};
}

} // namespace net
