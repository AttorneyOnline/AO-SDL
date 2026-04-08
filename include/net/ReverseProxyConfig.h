/**
 * @file ReverseProxyConfig.h
 * @brief Configuration for reverse proxy support.
 *
 * Shared between WebSocketServer, HTTP server, and ServerSettings.
 * Defined standalone to avoid circular dependencies.
 */
#pragma once

#include <string>
#include <vector>

/// Configuration for reverse proxy IP extraction.
struct ReverseProxyConfig {
    bool enabled = false;

    /// IP addresses or CIDR ranges of trusted reverse proxies.
    /// Only when the TCP peer matches an entry here do we extract
    /// the real client IP from headers or PROXY protocol.
    std::vector<std::string> trusted_proxies;

    /// HTTP headers to check for the real client IP, in priority order.
    /// First match wins. Default: X-Forwarded-For, then X-Real-IP.
    std::vector<std::string> header_priority = {"X-Forwarded-For", "X-Real-IP"};

    /// Enable HAProxy PROXY protocol v1/v2 parsing before HTTP.
    bool proxy_protocol = false;
};
