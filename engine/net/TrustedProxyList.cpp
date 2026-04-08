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

/// Trim whitespace from both ends of a string.
static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    auto end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

/// Validate that a string is a valid IPv4 or IPv6 address via inet_pton.
static bool is_valid_ip(const std::string& ip) {
    struct in_addr v4{};
    struct in6_addr v6{};
    return inet_pton(AF_INET, ip.c_str(), &v4) == 1 || inet_pton(AF_INET6, ip.c_str(), &v6) == 1;
}

std::string extract_forwarded_ip(const std::string& header_value, const TrustedProxyList* trusted) {
    if (header_value.empty())
        return "";

    // Split on commas into a list of IPs
    std::vector<std::string> ips;
    size_t pos = 0;
    while (pos < header_value.size()) {
        auto comma = header_value.find(',', pos);
        if (comma == std::string::npos) {
            ips.push_back(trim(header_value.substr(pos)));
            break;
        }
        ips.push_back(trim(header_value.substr(pos, comma - pos)));
        pos = comma + 1;
    }

    if (ips.empty())
        return "";

    // Single IP (e.g. X-Real-IP): validate and return directly
    if (ips.size() == 1) {
        auto& ip = ips[0];
        return is_valid_ip(ip) ? ip : "";
    }

    // Multiple IPs (X-Forwarded-For chain):
    // Walk right-to-left, skip trusted proxies, return first untrusted IP.
    // This prevents spoofing when proxies append to XFF.
    if (trusted) {
        for (int i = static_cast<int>(ips.size()) - 1; i >= 0; --i) {
            auto& ip = ips[i];
            if (ip.empty() || !is_valid_ip(ip))
                continue;
            if (!trusted->is_trusted(ip))
                return ip;
        }
        // All IPs in the chain are trusted — shouldn't happen in practice.
        // Return the leftmost as fallback.
    }

    // No trusted list or fallback: return leftmost (only safe if proxy overwrites XFF)
    auto& first = ips[0];
    return is_valid_ip(first) ? first : "";
}

} // namespace net
