/**
 * @file AwsSigV4.h
 * @brief Minimal AWS Signature Version 4 signing utility.
 *
 * Signs HTTP requests for AWS services using the SigV4 algorithm.
 * Depends only on Crypto.h (SHA-256, HMAC-SHA256) — no AWS SDK required.
 *
 * Reference: https://docs.aws.amazon.com/general/latest/gr/sigv4_signing.html
 */
#pragma once

#include "utils/Crypto.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace aws {

/// AWS credentials.
///
/// For static IAM user access keys, populate `access_key_id` and
/// `secret_access_key` only. For temporary credentials (STS, EC2 instance
/// role via IMDS, etc.), also populate `session_token` — SigV4 will
/// include it in the `x-amz-security-token` header and any consumer
/// (AWS service) will accept the request.
struct Credentials {
    std::string access_key_id;
    std::string secret_access_key;

    /// Session token for temporary credentials. Non-empty for anything
    /// issued by STS / IMDS. Signed into the request as
    /// `x-amz-security-token`. Leave empty for long-lived IAM user keys.
    std::string session_token;
};

/// A request to be signed. Caller fills in the fields, then calls sign().
struct SignableRequest {
    std::string method; // "POST", "GET", etc.
    std::string uri;    // "/", "/2014-03-28/...", etc.
    std::string query;  // query string without leading '?', or empty

    /// Headers to include. Must contain "host" at minimum.
    /// Keys should be lowercase.
    std::map<std::string, std::string> headers;

    std::string body; // request body (may be empty)
};

/// Result of signing — the headers that need to be added to the HTTP request.
struct SignedHeaders {
    std::string authorization;        // full "AWS4-HMAC-SHA256 Credential=... Signature=..." value
    std::string x_amz_date;           // "YYYYMMDDTHHMMSSZ"
    std::string x_amz_content_sha256; // hex-encoded SHA256 of body

    /// Echo of `Credentials::session_token` when signing with
    /// temporary credentials. Empty string for static IAM user keys.
    /// Callers MUST add this as the `X-Amz-Security-Token` header on
    /// the outgoing request whenever it's non-empty — AWS rejects
    /// temporary-cred requests that omit it.
    std::string x_amz_security_token;
};

// -- URI encoding (RFC 3986) --------------------------------------------------

namespace detail {

inline bool is_unreserved(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
           c == '.' || c == '~';
}

inline std::string uri_encode(const std::string& s, bool encode_slash = true) {
    std::ostringstream out;
    for (unsigned char c : s) {
        if (is_unreserved(c) || (!encode_slash && c == '/'))
            out << c;
        else
            out << '%' << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (int)c;
    }
    return out.str();
}

inline std::string to_lowercase(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

/// Format a time_point as "YYYYMMDDTHHMMSSZ".
inline std::string format_iso8601(std::chrono::system_clock::time_point tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &tm);
    return buf;
}

/// Extract date portion "YYYYMMDD" from an ISO 8601 timestamp.
inline std::string date_from_iso8601(const std::string& iso) {
    return iso.substr(0, 8);
}

} // namespace detail

// -- Signing ------------------------------------------------------------------

/// Sign a request using AWS Signature Version 4.
///
/// @param req      The request to sign (method, uri, headers, body).
/// @param creds    AWS access key and secret.
/// @param region   AWS region (e.g., "us-east-1").
/// @param service  AWS service name (e.g., "logs").
/// @param now      Timestamp to use (default: current UTC time).
/// @return         Headers to add to the outgoing HTTP request.
inline SignedHeaders sign(const SignableRequest& req, const Credentials& creds, const std::string& region,
                          const std::string& service,
                          std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) {

    std::string timestamp = detail::format_iso8601(now);
    std::string datestamp = detail::date_from_iso8601(timestamp);

    // --- Step 1: Canonical request ---

    // Canonical URI
    std::string canonical_uri = detail::uri_encode(req.uri, false);
    if (canonical_uri.empty())
        canonical_uri = "/";

    // Canonical query string (parameters sorted by key)
    std::string canonical_querystring = req.query; // caller provides pre-sorted or empty

    // Canonical headers (sorted by lowercase key, trimmed values)
    std::map<std::string, std::string> canonical_header_map;
    for (auto& [k, v] : req.headers)
        canonical_header_map[detail::to_lowercase(k)] = v;
    // Ensure x-amz-date is present
    canonical_header_map["x-amz-date"] = timestamp;

    // Payload hash
    std::string payload_hash = crypto::sha256(req.body);
    canonical_header_map["x-amz-content-sha256"] = payload_hash;

    // Temporary-credential security token (STS/IMDS). When present, it
    // MUST be included in the signed headers — AWS rejects requests
    // that carry a session token in an unsigned header. Adding it here
    // (before the canonical_headers loop below) automatically folds it
    // into both the canonical header block and the SignedHeaders list.
    if (!creds.session_token.empty())
        canonical_header_map["x-amz-security-token"] = creds.session_token;

    std::string canonical_headers;
    std::string signed_headers;
    for (auto& [k, v] : canonical_header_map) {
        canonical_headers += k + ":" + v + "\n";
        if (!signed_headers.empty())
            signed_headers += ";";
        signed_headers += k;
    }

    std::string canonical_request = req.method + "\n" + canonical_uri + "\n" + canonical_querystring + "\n" +
                                    canonical_headers + "\n" + signed_headers + "\n" + payload_hash;

    // --- Step 2: String to sign ---

    std::string credential_scope = datestamp + "/" + region + "/" + service + "/aws4_request";
    std::string string_to_sign =
        "AWS4-HMAC-SHA256\n" + timestamp + "\n" + credential_scope + "\n" + crypto::sha256(canonical_request);

    // --- Step 3: Signing key ---
    //   kDate    = HMAC("AWS4" + secret, datestamp)
    //   kRegion  = HMAC(kDate, region)
    //   kService = HMAC(kRegion, service)
    //   kSigning = HMAC(kService, "aws4_request")

    std::string initial_key = "AWS4" + creds.secret_access_key;
    std::vector<uint8_t> k_date =
        crypto::hmac_sha256(std::vector<uint8_t>(initial_key.begin(), initial_key.end()), datestamp);
    std::vector<uint8_t> k_region = crypto::hmac_sha256(k_date, region);
    std::vector<uint8_t> k_service = crypto::hmac_sha256(k_region, service);
    std::vector<uint8_t> k_signing = crypto::hmac_sha256(k_service, std::string("aws4_request"));

    // --- Step 4: Signature ---

    auto signature_bytes = crypto::hmac_sha256(k_signing, string_to_sign);
    // Convert to hex
    uint32_t words[8];
    for (int i = 0; i < 8; i++)
        words[i] = crypto::detail::read_be32(signature_bytes.data() + 4 * i);
    std::string signature = crypto::detail::to_hex(words, 8);

    // --- Step 5: Authorization header ---

    std::string authorization = "AWS4-HMAC-SHA256 Credential=" + creds.access_key_id + "/" + credential_scope +
                                ", SignedHeaders=" + signed_headers + ", Signature=" + signature;

    return {authorization, timestamp, payload_hash, creds.session_token};
}

} // namespace aws
