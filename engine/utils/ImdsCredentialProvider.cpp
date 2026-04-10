#include "utils/ImdsCredentialProvider.h"

#include "net/Http.h"
#include "utils/Log.h"

#include <json.hpp>

#include <ctime>
#include <stdexcept>

namespace aws {

namespace {

/// Link-local IMDS host. Never changes, never needs DNS resolution.
constexpr const char* kImdsBaseUrl = "http://169.254.169.254";

/// Network I/O budget for a single IMDS round-trip. IMDS lives on the
/// link-local interface with single-digit-ms latency; if it's slower
/// than this something is deeply wrong (container networking
/// misconfigured, IMDS disabled, etc.) and we'd rather fail fast than
/// block the caller.
constexpr int kImdsTimeoutSeconds = 2;

/// Production HTTP transport. A thin adapter over http::Client that
/// hits the IMDS link-local endpoint with a short timeout and the
/// IMDSv2 headers. Stateless — a fresh Client per call is fine since
/// IMDS doesn't benefit from keep-alive and the short-lived socket
/// avoids holding FDs between refreshes.
class DefaultImdsHttpClient : public ImdsHttpClient {
  public:
    std::optional<std::string> put_token(int ttl_seconds) override {
        http::Client client(kImdsBaseUrl);
        client.set_connection_timeout(kImdsTimeoutSeconds, 0);
        client.set_read_timeout(kImdsTimeoutSeconds, 0);
        http::Headers headers = {
            {"X-aws-ec2-metadata-token-ttl-seconds", std::to_string(ttl_seconds)},
        };
        // Empty body — IMDSv2 tokens are issued on any PUT to this
        // path, the body is ignored. Pass nullptr/0/"" so httplib
        // doesn't set an unwanted Content-Type.
        auto result = client.Put("/latest/api/token", headers, nullptr, 0, "");
        if (!result || result->status != 200)
            return std::nullopt;
        return result->body;
    }

    std::optional<std::string> get_role_name(const std::string& token) override {
        http::Client client(kImdsBaseUrl);
        client.set_connection_timeout(kImdsTimeoutSeconds, 0);
        client.set_read_timeout(kImdsTimeoutSeconds, 0);
        http::Headers headers = {
            {"X-aws-ec2-metadata-token", token},
        };
        auto result = client.Get("/latest/meta-data/iam/security-credentials/", headers);
        if (!result || result->status != 200)
            return std::nullopt;
        // Response is a single role name possibly followed by other
        // role names on subsequent lines. We use the first line —
        // instance profiles typically have exactly one role and the
        // multi-role case is not supported by the AWS SDK either.
        auto body = result->body;
        auto newline = body.find('\n');
        if (newline != std::string::npos)
            body.resize(newline);
        while (!body.empty() && (body.back() == '\r' || body.back() == ' '))
            body.pop_back();
        if (body.empty())
            return std::nullopt;
        return body;
    }

    std::optional<std::string> get_credentials_json(const std::string& token, const std::string& role) override {
        http::Client client(kImdsBaseUrl);
        client.set_connection_timeout(kImdsTimeoutSeconds, 0);
        client.set_read_timeout(kImdsTimeoutSeconds, 0);
        http::Headers headers = {
            {"X-aws-ec2-metadata-token", token},
        };
        auto result = client.Get("/latest/meta-data/iam/security-credentials/" + role, headers);
        if (!result || result->status != 200)
            return std::nullopt;
        return result->body;
    }
};

/// Parse an ISO-8601 timestamp like "2026-04-10T21:05:12Z" into a
/// `system_clock::time_point`. IMDS always emits UTC with a trailing
/// Z, no fractional seconds, so we handle that shape specifically
/// rather than dragging in a full ISO-8601 parser.
///
/// Returns nullopt on any format deviation.
std::optional<std::chrono::system_clock::time_point> parse_iso8601_utc(const std::string& s) {
    // Expected layout: YYYY-MM-DDTHH:MM:SSZ (20 characters).
    if (s.size() < 20 || s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' || s[16] != ':')
        return std::nullopt;

    std::tm tm{};
    try {
        tm.tm_year = std::stoi(s.substr(0, 4)) - 1900;
        tm.tm_mon = std::stoi(s.substr(5, 2)) - 1;
        tm.tm_mday = std::stoi(s.substr(8, 2));
        tm.tm_hour = std::stoi(s.substr(11, 2));
        tm.tm_min = std::stoi(s.substr(14, 2));
        tm.tm_sec = std::stoi(s.substr(17, 2));
    }
    catch (const std::exception&) {
        return std::nullopt;
    }

    // timegm is POSIX — Windows uses _mkgmtime. We don't ship this
    // code on Windows (kagami CI is Linux/macOS for the AWS-facing
    // targets) but guard for portability in case that changes.
#ifdef _WIN32
    std::time_t tt = _mkgmtime(&tm);
#else
    std::time_t tt = timegm(&tm);
#endif
    if (tt == -1)
        return std::nullopt;
    return std::chrono::system_clock::from_time_t(tt);
}

} // namespace

std::unique_ptr<ImdsHttpClient> make_default_imds_http_client() {
    return std::make_unique<DefaultImdsHttpClient>();
}

// ---------------------------------------------------------------------------
// ImdsCredentialProvider
// ---------------------------------------------------------------------------

ImdsCredentialProvider::ImdsCredentialProvider() : http_(make_default_imds_http_client()) {
}

ImdsCredentialProvider::ImdsCredentialProvider(std::unique_ptr<ImdsHttpClient> http) : http_(std::move(http)) {
}

ImdsCredentialProvider::~ImdsCredentialProvider() = default;

Credentials ImdsCredentialProvider::get() {
    std::lock_guard lock(mu_);

    // Serve from cache if the current creds still have headroom over
    // the refresh buffer. Using the buffer rather than raw expiration
    // means we never hand a caller a credential that will expire mid-
    // request.
    if (cache_) {
        auto now = std::chrono::system_clock::now();
        if (now + kRefreshBuffer < cache_->expiration)
            return cache_->creds;
    }

    cache_ = fetch_locked();
    return cache_->creds;
}

void ImdsCredentialProvider::invalidate() {
    std::lock_guard lock(mu_);
    cache_.reset();
}

bool ImdsCredentialProvider::is_cached() const {
    std::lock_guard lock(mu_);
    if (!cache_)
        return false;
    auto now = std::chrono::system_clock::now();
    return now + kRefreshBuffer < cache_->expiration;
}

ImdsCredentialProvider::CachedCreds ImdsCredentialProvider::fetch_locked() {
    // --- Step 1: IMDSv2 session token ---
    auto token = http_->put_token(kImdsTokenTtlSeconds);
    if (!token)
        throw std::runtime_error("IMDS: failed to fetch session token (PUT /latest/api/token)");

    // --- Step 2: role name ---
    auto role = http_->get_role_name(*token);
    if (!role)
        throw std::runtime_error("IMDS: no IAM role attached to instance (or role discovery failed)");

    // --- Step 3: credentials JSON ---
    auto json = http_->get_credentials_json(*token, *role);
    if (!json)
        throw std::runtime_error("IMDS: failed to fetch credentials for role " + *role);

    auto parsed = parse_credentials_json(*json);
    if (!parsed)
        throw std::runtime_error("IMDS: credentials JSON for role " + *role + " was malformed or missing fields");

    Log::log_print(INFO, "IMDS: fetched credentials for role %s (expires in ~%lld seconds)", role->c_str(),
                   static_cast<long long>(std::chrono::duration_cast<std::chrono::seconds>(
                                              parsed->second - std::chrono::system_clock::now())
                                              .count()));

    return {parsed->first, parsed->second};
}

std::optional<std::pair<Credentials, std::chrono::system_clock::time_point>>
ImdsCredentialProvider::parse_credentials_json(const std::string& json) {
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(json);
    }
    catch (const nlohmann::json::parse_error&) {
        return std::nullopt;
    }

    // Required fields. Missing any of them makes the credentials
    // unusable — we refuse to return a half-populated struct.
    if (!doc.is_object() || !doc.contains("AccessKeyId") || !doc.contains("SecretAccessKey") ||
        !doc.contains("Token") || !doc.contains("Expiration"))
        return std::nullopt;

    if (!doc["AccessKeyId"].is_string() || !doc["SecretAccessKey"].is_string() || !doc["Token"].is_string() ||
        !doc["Expiration"].is_string())
        return std::nullopt;

    Credentials creds;
    creds.access_key_id = doc["AccessKeyId"].get<std::string>();
    creds.secret_access_key = doc["SecretAccessKey"].get<std::string>();
    creds.session_token = doc["Token"].get<std::string>();

    auto expiration = parse_iso8601_utc(doc["Expiration"].get<std::string>());
    if (!expiration)
        return std::nullopt;

    // Sanity check: reject already-expired credentials. IMDS should
    // never hand these out, but treating a stale response as valid
    // would push the "bad creds" error to the first AWS call instead
    // of surfacing it here at fetch time.
    if (*expiration <= std::chrono::system_clock::now())
        return std::nullopt;

    return std::make_pair(std::move(creds), *expiration);
}

} // namespace aws
