/**
 * @file ProxyProtocol.h
 * @brief HAProxy PROXY protocol v1 and v2 parser.
 *
 * Parses the PROXY protocol header that reverse proxies prepend to
 * TCP connections to convey the original client address.
 *
 * v1 is a single ASCII line: "PROXY TCP4 1.2.3.4 5.6.7.8 56324 443\r\n"
 * v2 is a binary header with a 12-byte signature followed by address data.
 *
 * See: https://www.haproxy.org/download/2.9/doc/proxy-protocol.txt
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace net {

struct ProxyProtocolResult {
    bool valid = false;          ///< True if a valid header was parsed
    bool need_more_data = false; ///< True if buffer is too short to determine
    std::string client_addr;     ///< Parsed client IP address
    uint16_t client_port = 0;    ///< Parsed client port
    std::string server_addr;     ///< Parsed server/proxy IP address
    uint16_t server_port = 0;    ///< Parsed server port
    size_t header_length = 0;    ///< Total bytes consumed by the header
};

/// Parse a PROXY protocol header (v1 or v2) from raw bytes.
///
/// If the data starts with "PROXY " (v1) or the v2 binary signature,
/// it will be parsed. Otherwise, returns {valid=false, need_more_data=false}
/// indicating this is a direct connection (no PROXY protocol).
///
/// Returns need_more_data=true if the buffer might contain a valid header
/// but is truncated.
ProxyProtocolResult parse_proxy_protocol(const uint8_t* data, size_t len);

} // namespace net
