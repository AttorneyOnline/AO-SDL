#include "net/TrustedProxyList.h"

#include "utils/Log.h"

#include <cctype>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace net {

// -- IPv4 helpers -------------------------------------------------------------

bool TrustedProxyList::parse_ipv4(const std::string& ip, uint32_t& out) {
    struct in_addr addr{};
    if (inet_pton(AF_INET, ip.c_str(), &addr) == 1) {
        out = ntohl(addr.s_addr);
        return true;
    }
    return false;
}

uint32_t TrustedProxyList::ipv4_mask(int prefix) {
    if (prefix <= 0)
        return 0;
    if (prefix >= 32)
        return 0xFFFFFFFF;
    return ~((1u << (32 - prefix)) - 1);
}

// -- IPv6 helpers -------------------------------------------------------------

bool TrustedProxyList::parse_ipv6(const std::string& ip, uint8_t out[16]) {
    struct in6_addr addr{};
    if (inet_pton(AF_INET6, ip.c_str(), &addr) == 1) {
        std::memcpy(out, &addr, 16);
        return true;
    }
    return false;
}

static bool ipv6_matches(const uint8_t addr[16], const uint8_t network[16], int prefix_len) {
    // Compare full bytes
    int full_bytes = prefix_len / 8;
    if (full_bytes > 0 && std::memcmp(addr, network, full_bytes) != 0)
        return false;

    // Compare remaining bits
    int remaining_bits = prefix_len % 8;
    if (remaining_bits > 0) {
        uint8_t mask = static_cast<uint8_t>(0xFF << (8 - remaining_bits));
        if ((addr[full_bytes] & mask) != (network[full_bytes] & mask))
            return false;
    }

    return true;
}

// -- Configuration ------------------------------------------------------------

void TrustedProxyList::configure(const std::vector<std::string>& entries) {
    ipv4_.clear();
    ipv6_.clear();

    for (auto& entry : entries) {
        if (entry.empty())
            continue;

        auto slash = entry.find('/');
        if (slash != std::string::npos) {
            // CIDR notation
            std::string ip_part = entry.substr(0, slash);
            int prefix = 0;
            try {
                prefix = std::stoi(entry.substr(slash + 1));
            }
            catch (...) {
                Log::log_print(WARNING, "TrustedProxyList: invalid CIDR: %s", entry.c_str());
                continue;
            }

            if (ip_part.find(':') != std::string::npos) {
                // IPv6 CIDR
                if (prefix < 0 || prefix > 128) {
                    Log::log_print(WARNING, "TrustedProxyList: invalid IPv6 prefix: %s", entry.c_str());
                    continue;
                }
                IPv6Entry e{};
                if (parse_ipv6(ip_part, e.addr)) {
                    e.prefix_len = prefix;
                    ipv6_.push_back(e);
                }
                else {
                    Log::log_print(WARNING, "TrustedProxyList: invalid IPv6 address: %s", entry.c_str());
                }
            }
            else {
                // IPv4 CIDR
                if (prefix < 0 || prefix > 32) {
                    Log::log_print(WARNING, "TrustedProxyList: invalid IPv4 prefix: %s", entry.c_str());
                    continue;
                }
                uint32_t addr = 0;
                if (parse_ipv4(ip_part, addr)) {
                    ipv4_.push_back({addr & ipv4_mask(prefix), ipv4_mask(prefix)});
                }
                else {
                    Log::log_print(WARNING, "TrustedProxyList: invalid IPv4 address: %s", entry.c_str());
                }
            }
        }
        else {
            // Exact IP
            if (entry.find(':') != std::string::npos) {
                // IPv6
                IPv6Entry e{};
                if (parse_ipv6(entry, e.addr)) {
                    e.prefix_len = 128; // exact match
                    ipv6_.push_back(e);
                }
                else {
                    Log::log_print(WARNING, "TrustedProxyList: invalid IPv6 address: %s", entry.c_str());
                }
            }
            else {
                // IPv4
                uint32_t addr = 0;
                if (parse_ipv4(entry, addr)) {
                    ipv4_.push_back({addr, 0xFFFFFFFF}); // /32 exact match
                }
                else {
                    Log::log_print(WARNING, "TrustedProxyList: invalid IPv4 address: %s", entry.c_str());
                }
            }
        }
    }

    Log::log_print(INFO, "TrustedProxyList: configured %zu IPv4 + %zu IPv6 entries", ipv4_.size(), ipv6_.size());
}

// -- Matching -----------------------------------------------------------------

bool TrustedProxyList::is_trusted(const std::string& ip) const {
    if (ip.empty())
        return false;

    if (ip.find(':') != std::string::npos) {
        // IPv6
        uint8_t addr[16];
        if (!parse_ipv6(ip, addr))
            return false;

        for (auto& entry : ipv6_) {
            if (ipv6_matches(addr, entry.addr, entry.prefix_len))
                return true;
        }
    }
    else {
        // IPv4
        uint32_t addr = 0;
        if (!parse_ipv4(ip, addr))
            return false;

        for (auto& entry : ipv4_) {
            if ((addr & entry.mask) == entry.addr)
                return true;
        }
    }

    return false;
}

bool TrustedProxyList::empty() const {
    return ipv4_.empty() && ipv6_.empty();
}

size_t TrustedProxyList::size() const {
    return ipv4_.size() + ipv6_.size();
}

// -- Header extraction --------------------------------------------------------

std::string extract_forwarded_ip(const std::string& header_value) {
    if (header_value.empty())
        return "";

    // For X-Forwarded-For: take the leftmost (first) IP
    // Format: "client, proxy1, proxy2"
    auto comma = header_value.find(',');
    std::string ip = (comma != std::string::npos) ? header_value.substr(0, comma) : header_value;

    // Trim whitespace
    auto start = ip.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    auto end = ip.find_last_not_of(" \t");
    return ip.substr(start, end - start + 1);
}

} // namespace net
