/**
 * @file ImdsCredentialProvider.h
 * @brief EC2 Instance Metadata Service (IMDSv2) credential provider.
 *
 * Fetches temporary AWS credentials from the role attached to the EC2
 * instance the process is running on, and caches them until they
 * expire. Intended as a drop-in replacement for baking long-lived IAM
 * user access keys into kagami.json — EC2 already hands out short-
 * lived rotating credentials via a link-local HTTP endpoint at
 * 169.254.169.254, and using them is the AWS-native way.
 *
 * IMDSv2 protocol (the version we implement — IMDSv1 is being
 * deprecated and recent AMIs enforce "tokens required"):
 *
 *   1. PUT http://169.254.169.254/latest/api/token
 *      X-aws-ec2-metadata-token-ttl-seconds: <int>
 *      → returns an opaque session token string.
 *
 *   2. GET http://169.254.169.254/latest/meta-data/iam/security-credentials/
 *      X-aws-ec2-metadata-token: <token>
 *      → returns the name of the role attached to the instance (one
 *        line of text). Empty response = instance has no role, which
 *        is a fatal error for this provider.
 *
 *   3. GET http://169.254.169.254/latest/meta-data/iam/security-credentials/<role>
 *      X-aws-ec2-metadata-token: <token>
 *      → returns a JSON blob:
 *          {
 *            "AccessKeyId": "ASIA...",
 *            "SecretAccessKey": "...",
 *            "Token": "...",            // x-amz-security-token
 *            "Expiration": "2026-04-10T21:05:12Z",
 *            ...
 *          }
 *
 * Credentials are cached and re-fetched opportunistically when the
 * caller requests them and the current time is within
 * `kRefreshBuffer` of the reported expiration — matching AWS SDK
 * default behavior.
 *
 * Thread-safety: concurrent `get()` calls share a mutex; exactly one
 * network fetch runs at a time. A successful fetch populates the
 * cache for all subsequent callers until near-expiration.
 *
 * Testability: the HTTP transport is abstracted behind
 * `ImdsHttpClient` so unit tests can inject canned responses. The
 * credentials JSON parser is exposed as a static method for direct
 * testing.
 */
#pragma once

#include "utils/AwsSigV4.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace aws {

/// Abstract HTTP transport for IMDSv2. Production code uses a plain
/// `http::Client` over `http://169.254.169.254`; tests inject a mock.
class ImdsHttpClient {
  public:
    virtual ~ImdsHttpClient() = default;

    /// PUT /latest/api/token with the TTL header.
    /// Returns the token body on HTTP 200, nullopt on any failure.
    virtual std::optional<std::string> put_token(int ttl_seconds) = 0;

    /// GET /latest/meta-data/iam/security-credentials/ with the token.
    /// Returns the role name on 200, nullopt on any failure.
    virtual std::optional<std::string> get_role_name(const std::string& token) = 0;

    /// GET /latest/meta-data/iam/security-credentials/<role> with the token.
    /// Returns the raw JSON body on 200, nullopt on any failure.
    virtual std::optional<std::string> get_credentials_json(const std::string& token, const std::string& role) = 0;
};

/// Default HTTP transport: hits 169.254.169.254 via plain HTTP with a
/// short timeout. IMDS lives on the link-local interface, there is no
/// TLS and no name resolution to worry about.
std::unique_ptr<ImdsHttpClient> make_default_imds_http_client();

/// Provides AWS credentials from the EC2 Instance Metadata Service.
class ImdsCredentialProvider {
  public:
    /// Refresh cached creds when they're within this window of expiring.
    /// 5 minutes matches the AWS SDK default. Large enough to avoid
    /// thundering-herd refreshes but small enough that clients never
    /// see actually-expired credentials.
    static constexpr auto kRefreshBuffer = std::chrono::minutes(5);

    /// IMDSv2 session-token TTL. 6 hours matches the AWS SDK default —
    /// IMDS itself allows up to 6 hours (21600 seconds).
    static constexpr int kImdsTokenTtlSeconds = 21600;

    /// Construct with the production HTTP transport.
    ImdsCredentialProvider();

    /// Construct with an injected HTTP transport (test hook).
    explicit ImdsCredentialProvider(std::unique_ptr<ImdsHttpClient> http);

    ~ImdsCredentialProvider();

    /// Return a currently-valid set of credentials. If the cache is
    /// fresh, returns a cached copy without hitting the network.
    /// Otherwise performs a synchronous IMDSv2 fetch under the lock.
    ///
    /// On any error (no role attached, IMDS unreachable, parse
    /// failure), throws `std::runtime_error` with a descriptive
    /// message. Callers should wrap this in a try/catch so a
    /// transient IMDS failure doesn't propagate out of background
    /// flush threads.
    Credentials get();

    /// Invalidate the cache. The next `get()` forces a fresh fetch.
    /// Useful in tests and on deliberate credential-rotation events.
    void invalidate();

    /// True if the cache currently holds valid, unexpired credentials.
    /// Primarily useful for tests and diagnostics.
    bool is_cached() const;

    // -- Internals exposed for testing --------------------------------

    /// Parse the IMDS credentials JSON blob into a Credentials struct
    /// and an absolute expiration time.
    ///
    /// Returns `nullopt` if the JSON is malformed, any of the required
    /// fields (AccessKeyId, SecretAccessKey, Token, Expiration) are
    /// missing, or the Expiration timestamp can't be parsed as
    /// ISO-8601.
    ///
    /// Exposed as a static method so tests can verify parser behavior
    /// without standing up the whole class.
    static std::optional<std::pair<Credentials, std::chrono::system_clock::time_point>>
    parse_credentials_json(const std::string& json);

  private:
    /// Cached credentials with their wall-clock expiration time.
    struct CachedCreds {
        Credentials creds;
        std::chrono::system_clock::time_point expiration;
    };

    /// Perform a fresh IMDSv2 round-trip. Caller must hold `mu_`.
    /// Throws `std::runtime_error` on any failure.
    CachedCreds fetch_locked();

    std::unique_ptr<ImdsHttpClient> http_;
    mutable std::mutex mu_;
    std::optional<CachedCreds> cache_;
};

} // namespace aws
