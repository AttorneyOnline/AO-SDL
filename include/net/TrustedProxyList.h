/**
 * @file TrustedProxyList.h
 * @brief CIDR-matching allowlist for reverse proxy trust.
 *
 * Parses a list of IP addresses and CIDR ranges at configure time.
 * is_trusted() performs O(N) matching where N is the entry count
 * (typically 1–3 for most deployments).
 *
 * Supports both IPv4 (1.2.3.4, 10.0.0.0/8) and IPv6 (::1, fd00::/8).
 *
 * Thread safety: configure() is not thread-safe with concurrent is_trusted()
 * calls. Call configure() once at startup before any queries.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace net {

class TrustedProxyList {
  public:
    TrustedProxyList() = default;

    /// Parse a list of IP addresses and/or CIDR ranges.
    /// Invalid entries are silently skipped (logged at WARNING).
    void configure(const std::vector<std::string>& entries);

    /// Check if an IP address is in the trusted list.
    /// Matches against exact IPs and CIDR ranges.
    bool is_trusted(const std::string& ip) const;

    /// Returns true if the list has any entries configured.
    bool empty() const;

    /// Number of configured entries.
    size_t size() const;

  private:
    struct IPv4Entry {
        uint32_t addr; ///< Host byte order
        uint32_t mask; ///< Host byte order
    };

    struct IPv6Entry {
        uint8_t addr[16];
        int prefix_len;
    };

    std::vector<IPv4Entry> ipv4_;
    std::vector<IPv6Entry> ipv6_;

    /// Parse an IPv4 address string into a uint32_t (host byte order).
    static bool parse_ipv4(const std::string& ip, uint32_t& out);

    /// Parse an IPv6 address string into a 16-byte array.
    static bool parse_ipv6(const std::string& ip, uint8_t out[16]);

    /// Generate an IPv4 mask from a prefix length.
    static uint32_t ipv4_mask(int prefix);
};

/// Extract the real client IP from an X-Forwarded-For or X-Real-IP header value.
/// For XFF, returns the leftmost (first) IP. For X-Real-IP, returns the value directly.
/// Returns empty string if the header value is empty or unparseable.
std::string extract_forwarded_ip(const std::string& header_value);

} // namespace net
