#include "utils/ImdsCredentialProvider.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <utility>

using aws::ImdsCredentialProvider;
using aws::ImdsHttpClient;

// Helper: format a future UTC time as ISO-8601 "YYYY-MM-DDTHH:MM:SSZ"
// for use as an Expiration field in the credentials JSON. The
// provider's parser accepts this shape and no other.
static std::string iso8601_in(std::chrono::seconds delta) {
    auto tp = std::chrono::system_clock::now() + delta;
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

// ---------------------------------------------------------------------------
// parse_credentials_json — static parser behavior
// ---------------------------------------------------------------------------

TEST(ImdsCredentialProviderParse, ValidCredentials) {
    auto json = R"({
        "AccessKeyId": "ASIATESTKEY",
        "SecretAccessKey": "secretvalue",
        "Token": "sessiontokenvalue",
        "Expiration": ")" +
                iso8601_in(std::chrono::hours(1)) +
                R"("
    })";
    auto result = ImdsCredentialProvider::parse_credentials_json(json);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->first.access_key_id, "ASIATESTKEY");
    EXPECT_EQ(result->first.secret_access_key, "secretvalue");
    EXPECT_EQ(result->first.session_token, "sessiontokenvalue");
    // Expiration parsed as roughly 1 hour from now (allow ±5s for
    // test scheduling jitter + second-granularity timestamps).
    auto delta =
        std::chrono::duration_cast<std::chrono::seconds>(result->second - std::chrono::system_clock::now()).count();
    EXPECT_GE(delta, 3595);
    EXPECT_LE(delta, 3605);
}

TEST(ImdsCredentialProviderParse, RejectsMalformedJson) {
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json("not json at all").has_value());
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json("").has_value());
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json("[]").has_value());
}

TEST(ImdsCredentialProviderParse, RejectsMissingAccessKeyId) {
    auto json = R"({
        "SecretAccessKey": "s",
        "Token": "t",
        "Expiration": ")" +
                iso8601_in(std::chrono::hours(1)) +
                R"("
    })";
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json(json).has_value());
}

TEST(ImdsCredentialProviderParse, RejectsMissingSecretAccessKey) {
    auto json = R"({
        "AccessKeyId": "a",
        "Token": "t",
        "Expiration": ")" +
                iso8601_in(std::chrono::hours(1)) +
                R"("
    })";
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json(json).has_value());
}

TEST(ImdsCredentialProviderParse, RejectsMissingToken) {
    // IMDS always returns a session token for temporary creds; a
    // payload without one is malformed and we refuse to use it
    // because signing without the security token would produce a
    // request AWS rejects.
    auto json = R"({
        "AccessKeyId": "a",
        "SecretAccessKey": "s",
        "Expiration": ")" +
                iso8601_in(std::chrono::hours(1)) +
                R"("
    })";
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json(json).has_value());
}

TEST(ImdsCredentialProviderParse, RejectsMissingExpiration) {
    auto json = R"({
        "AccessKeyId": "a",
        "SecretAccessKey": "s",
        "Token": "t"
    })";
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json(json).has_value());
}

TEST(ImdsCredentialProviderParse, RejectsBadExpirationFormat) {
    // Not ISO-8601 with trailing Z.
    auto json = R"({
        "AccessKeyId": "a",
        "SecretAccessKey": "s",
        "Token": "t",
        "Expiration": "next tuesday"
    })";
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json(json).has_value());
}

TEST(ImdsCredentialProviderParse, RejectsAlreadyExpired) {
    // IMDS should never hand us an already-expired timestamp, but
    // treating it as valid would hide the problem until the first
    // CloudWatch call fails. Better to reject at parse time.
    auto json = R"({
        "AccessKeyId": "a",
        "SecretAccessKey": "s",
        "Token": "t",
        "Expiration": ")" +
                iso8601_in(std::chrono::hours(-1)) +
                R"("
    })";
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json(json).has_value());
}

TEST(ImdsCredentialProviderParse, RejectsNonStringFields) {
    // A numeric AccessKeyId is malformed even though the JSON parses.
    auto json = R"({
        "AccessKeyId": 12345,
        "SecretAccessKey": "s",
        "Token": "t",
        "Expiration": ")" +
                iso8601_in(std::chrono::hours(1)) +
                R"("
    })";
    EXPECT_FALSE(ImdsCredentialProvider::parse_credentials_json(json).has_value());
}

// ---------------------------------------------------------------------------
// End-to-end provider behavior with injected mock HTTP
// ---------------------------------------------------------------------------

/// Mock HTTP transport that returns canned responses and counts
/// method calls so tests can assert on cache-hit vs. fetch behavior.
class MockImdsHttp : public ImdsHttpClient {
  public:
    std::optional<std::string> token_response = std::string("mock-token");
    std::optional<std::string> role_response = std::string("kagami-role");
    std::optional<std::string> credentials_response;

    int put_token_calls = 0;
    int get_role_calls = 0;
    int get_credentials_calls = 0;

    std::optional<std::string> put_token(int /*ttl*/) override {
        ++put_token_calls;
        return token_response;
    }

    std::optional<std::string> get_role_name(const std::string& token) override {
        ++get_role_calls;
        last_token = token;
        return role_response;
    }

    std::optional<std::string> get_credentials_json(const std::string& token, const std::string& role) override {
        ++get_credentials_calls;
        last_token = token;
        last_role = role;
        return credentials_response;
    }

    std::string last_token;
    std::string last_role;
};

/// Build a valid credentials JSON for tests. Defaults to creds that
/// expire 1 hour in the future, which is well outside the
/// kRefreshBuffer and should be cached cleanly.
static std::string valid_credentials_json(std::chrono::seconds ttl = std::chrono::hours(1)) {
    return R"({
        "AccessKeyId": "ASIATEST",
        "SecretAccessKey": "secret",
        "Token": "token",
        "Expiration": ")" +
           iso8601_in(ttl) +
           R"("
    })";
}

TEST(ImdsCredentialProvider, HappyPath) {
    auto mock = std::make_unique<MockImdsHttp>();
    auto* mock_ptr = mock.get();
    mock->credentials_response = valid_credentials_json();

    ImdsCredentialProvider provider(std::move(mock));
    auto creds = provider.get();
    EXPECT_EQ(creds.access_key_id, "ASIATEST");
    EXPECT_EQ(creds.secret_access_key, "secret");
    EXPECT_EQ(creds.session_token, "token");
    EXPECT_EQ(mock_ptr->put_token_calls, 1);
    EXPECT_EQ(mock_ptr->get_role_calls, 1);
    EXPECT_EQ(mock_ptr->get_credentials_calls, 1);
    EXPECT_EQ(mock_ptr->last_role, "kagami-role");
    EXPECT_EQ(mock_ptr->last_token, "mock-token");
    EXPECT_TRUE(provider.is_cached());
}

TEST(ImdsCredentialProvider, CacheHitSkipsHttp) {
    auto mock = std::make_unique<MockImdsHttp>();
    auto* mock_ptr = mock.get();
    mock->credentials_response = valid_credentials_json();

    ImdsCredentialProvider provider(std::move(mock));
    provider.get(); // warm cache
    provider.get();
    provider.get();
    // Still only one round-trip total, subsequent calls are cached.
    EXPECT_EQ(mock_ptr->put_token_calls, 1);
    EXPECT_EQ(mock_ptr->get_role_calls, 1);
    EXPECT_EQ(mock_ptr->get_credentials_calls, 1);
}

TEST(ImdsCredentialProvider, InvalidateForcesRefetch) {
    auto mock = std::make_unique<MockImdsHttp>();
    auto* mock_ptr = mock.get();
    mock->credentials_response = valid_credentials_json();

    ImdsCredentialProvider provider(std::move(mock));
    provider.get();
    EXPECT_TRUE(provider.is_cached());
    provider.invalidate();
    EXPECT_FALSE(provider.is_cached());
    provider.get();
    EXPECT_EQ(mock_ptr->put_token_calls, 2);
    EXPECT_EQ(mock_ptr->get_role_calls, 2);
    EXPECT_EQ(mock_ptr->get_credentials_calls, 2);
}

TEST(ImdsCredentialProvider, NearExpirationRefetches) {
    auto mock = std::make_unique<MockImdsHttp>();
    auto* mock_ptr = mock.get();
    // Credentials that expire just INSIDE the 5-minute refresh buffer.
    // The provider should treat these as stale on first get() and
    // immediately hit IMDS again on the next call.
    mock->credentials_response = valid_credentials_json(std::chrono::minutes(2));

    ImdsCredentialProvider provider(std::move(mock));
    provider.get(); // first fetch — creds are inside the buffer
    // Even though we just fetched, the cache is considered stale
    // because now + kRefreshBuffer > expiration. Next get() should
    // trigger another fetch.
    EXPECT_FALSE(provider.is_cached());
    provider.get();
    EXPECT_EQ(mock_ptr->put_token_calls, 2);
}

TEST(ImdsCredentialProvider, ThrowsOnTokenFetchFailure) {
    auto mock = std::make_unique<MockImdsHttp>();
    mock->token_response = std::nullopt;
    ImdsCredentialProvider provider(std::move(mock));
    EXPECT_THROW(provider.get(), std::runtime_error);
}

TEST(ImdsCredentialProvider, ThrowsOnRoleDiscoveryFailure) {
    auto mock = std::make_unique<MockImdsHttp>();
    mock->role_response = std::nullopt;
    ImdsCredentialProvider provider(std::move(mock));
    EXPECT_THROW(provider.get(), std::runtime_error);
}

TEST(ImdsCredentialProvider, ThrowsOnCredentialsFetchFailure) {
    auto mock = std::make_unique<MockImdsHttp>();
    mock->credentials_response = std::nullopt;
    ImdsCredentialProvider provider(std::move(mock));
    EXPECT_THROW(provider.get(), std::runtime_error);
}

TEST(ImdsCredentialProvider, ThrowsOnMalformedCredentialsJson) {
    auto mock = std::make_unique<MockImdsHttp>();
    mock->credentials_response = std::string("{this is: not] valid json");
    ImdsCredentialProvider provider(std::move(mock));
    EXPECT_THROW(provider.get(), std::runtime_error);
}

TEST(ImdsCredentialProvider, TransientFailureLeavesCacheEmpty) {
    // If the first fetch fails, subsequent get() calls should keep
    // retrying IMDS rather than caching the failure.
    auto mock = std::make_unique<MockImdsHttp>();
    auto* mock_ptr = mock.get();
    mock->credentials_response = std::nullopt;

    ImdsCredentialProvider provider(std::move(mock));
    EXPECT_THROW(provider.get(), std::runtime_error);
    EXPECT_FALSE(provider.is_cached());
    // Now simulate IMDS coming back online.
    mock_ptr->credentials_response = valid_credentials_json();
    auto creds = provider.get();
    EXPECT_EQ(creds.access_key_id, "ASIATEST");
    EXPECT_TRUE(provider.is_cached());
}
