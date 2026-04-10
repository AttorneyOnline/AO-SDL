/**
 * @file RemoteClassifier.h
 * @brief HTTP client for remote content moderation APIs.
 *
 * This is Layer 2 of the moderation stack. It wraps a synchronous POST
 * to a moderation-classifier endpoint (currently only OpenAI's
 * `omni-moderation-latest`) and returns per-axis scores suitable for
 * folding into ModerationAxisScores.
 *
 * The client is intentionally synchronous from the packet handler's
 * perspective. ContentModerator::check() enforces a configurable
 * deadline (RemoteClassifierConfig::timeout_ms) and falls back to a
 * zero-score result if the call is too slow or fails. fail_open=true
 * (the default) means an outage at OpenAI does not brick chat —
 * Layer 1 still runs and carries the load.
 */
#pragma once

#include "moderation/ContentModerationConfig.h"
#include "moderation/ModerationTypes.h"

#include <memory>
#include <string>

namespace http {
class Client;
}

namespace moderation {

struct RemoteClassifierResult {
    bool ok = false;             ///< True on 2xx response with parseable body.
    ModerationAxisScores scores; ///< Populated from response; zero if ok=false.
    std::string error;           ///< Human-readable error on !ok.
    int http_status = 0;         ///< Raw HTTP status, or 0 for transport failure.
    int64_t duration_ms = 0;     ///< Wall-clock duration of the call.
};

/// Trait-style transport so tests can swap in a mock without touching
/// the network. The real implementation uses http::Client under the
/// hood; the mock is a plain std::function.
class RemoteClassifierTransport {
  public:
    virtual ~RemoteClassifierTransport() = default;

    /// Returns (http_status, body). Status 0 means transport failure
    /// (connection error, timeout, etc.); body then holds the reason.
    virtual std::pair<int, std::string> post_json(const std::string& url, const std::string& bearer_token,
                                                  const std::string& body, int timeout_ms) = 0;
};

/// Default transport: uses http::Client.
std::unique_ptr<RemoteClassifierTransport> make_http_transport();

class RemoteClassifier {
  public:
    RemoteClassifier();
    ~RemoteClassifier();

    RemoteClassifier(const RemoteClassifier&) = delete;
    RemoteClassifier& operator=(const RemoteClassifier&) = delete;

    void configure(const RemoteClassifierConfig& cfg);

    /// Inject a transport. Takes ownership. For production leave
    /// unset; the default constructor uses make_http_transport().
    void set_transport(std::unique_ptr<RemoteClassifierTransport> transport);

    /// Is the layer actually ready to make calls? True only if the
    /// config has enabled=true AND a non-empty api_key.
    bool is_active() const;

    /// Synchronously classify a message. Enforces the configured
    /// timeout. On any failure, returns a result with ok=false; the
    /// caller decides whether to treat that as fail-open (Layer 1 only)
    /// or fail-closed.
    RemoteClassifierResult classify(const std::string& text);

  private:
    RemoteClassifierConfig cfg_;
    std::unique_ptr<RemoteClassifierTransport> transport_;
};

/// Parse an OpenAI `/v1/moderations` response body into axis scores.
/// Exposed as a free function so tests can feed it canned JSON without
/// a live HTTP roundtrip.
bool parse_openai_response(const std::string& body, ModerationAxisScores& out, std::string& error);

} // namespace moderation
