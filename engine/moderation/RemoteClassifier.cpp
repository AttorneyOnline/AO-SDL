#include "moderation/RemoteClassifier.h"

#include "net/Http.h"

#include <json.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace moderation {

namespace {

class HttpTransport : public RemoteClassifierTransport {
  public:
    std::pair<int, std::string> post_json(const std::string& url, const std::string& bearer_token,
                                          const std::string& body, int timeout_ms) override {
        // Reuse the http::Client per base URL so the TLS handshake
        // only happens on the first call. Without this, a fresh
        // client per request costs 1-2 seconds on arm64 (mostly TLS
        // setup + OpenAI's DNS/LB warmup), which blows through any
        // reasonable per-call timeout.
        auto split = split_base(url);
        if (split.first.empty()) {
            return {0, "malformed url: " + url};
        }

        std::shared_ptr<http::Client> client;
        {
            std::lock_guard lock(clients_mu_);
            auto& slot = clients_[split.first];
            if (!slot) {
                slot = std::make_shared<http::Client>(split.first);
                // Keep-alive is httplib default; the client persists
                // between calls while this HttpTransport lives.
                slot->set_keep_alive(true);
            }
            client = slot;
        }
        client->set_connection_timeout(timeout_ms / 1000, (timeout_ms % 1000) * 1000);
        client->set_read_timeout(timeout_ms / 1000, (timeout_ms % 1000) * 1000);

        http::Headers headers = {
            {"Authorization", "Bearer " + bearer_token},
        };
        auto result = client->Post(split.second, headers, body, "application/json");
        if (!result)
            return {0, "transport failure: " + http::to_string(result.error())};
        return {result->status, result->body};
    }

  private:
    /// Split a URL like "https://api.openai.com/v1/moderations" into
    /// {"https://api.openai.com", "/v1/moderations"}.
    static std::pair<std::string, std::string> split_base(const std::string& url) {
        auto scheme_end = url.find("://");
        if (scheme_end == std::string::npos)
            return {"", ""};
        auto host_start = scheme_end + 3;
        auto path_start = url.find('/', host_start);
        if (path_start == std::string::npos)
            return {url, "/"};
        return {url.substr(0, path_start), url.substr(path_start)};
    }

    std::mutex clients_mu_;
    std::unordered_map<std::string, std::shared_ptr<http::Client>> clients_;
};

} // namespace

std::unique_ptr<RemoteClassifierTransport> make_http_transport() {
    return std::make_unique<HttpTransport>();
}

RemoteClassifier::RemoteClassifier() : transport_(make_http_transport()) {
}

RemoteClassifier::~RemoteClassifier() = default;

void RemoteClassifier::configure(const RemoteClassifierConfig& cfg) {
    cfg_ = cfg;
}

void RemoteClassifier::set_transport(std::unique_ptr<RemoteClassifierTransport> transport) {
    transport_ = std::move(transport);
}

bool RemoteClassifier::is_active() const {
    return cfg_.enabled && !cfg_.api_key.empty() && transport_ != nullptr;
}

RemoteClassifierResult RemoteClassifier::classify(const std::string& text) {
    RemoteClassifierResult out;
    if (!is_active()) {
        out.error = "layer disabled";
        return out;
    }

    // Build the JSON body. OpenAI's omni-moderation endpoint accepts
    // `input` as a string or array of strings. We send a single string
    // to keep token budgets predictable.
    nlohmann::json body_json{
        {"model", cfg_.model},
        {"input", text},
    };
    const std::string body = body_json.dump();

    const auto start = std::chrono::steady_clock::now();
    auto [status, resp_body] = transport_->post_json(cfg_.endpoint, cfg_.api_key, body, cfg_.timeout_ms);
    out.http_status = status;
    out.duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    if (status == 0) {
        out.error = resp_body;
        return out;
    }
    if (status < 200 || status >= 300) {
        out.error = "http " + std::to_string(status) + ": " +
                    (resp_body.size() > 200 ? resp_body.substr(0, 200) + "..." : resp_body);
        return out;
    }

    std::string parse_err;
    if (!parse_openai_response(resp_body, out.scores, parse_err)) {
        out.error = "parse: " + parse_err;
        return out;
    }
    out.ok = true;
    return out;
}

bool parse_openai_response(const std::string& body, ModerationAxisScores& out, std::string& error) {
    try {
        auto j = nlohmann::json::parse(body);
        // OpenAI moderation response shape:
        //   { "id": "...", "model": "...", "results": [ {
        //       "flagged": bool,
        //       "categories": { "sexual": bool, "hate": bool, ... },
        //       "category_scores": { "sexual": 0.02, "hate": 0.001, ... }
        //   } ] }
        if (!j.contains("results") || !j["results"].is_array() || j["results"].empty()) {
            error = "missing results[]";
            return false;
        }
        const auto& r0 = j["results"][0];
        if (!r0.contains("category_scores")) {
            error = "missing category_scores";
            return false;
        }
        const auto& scores = r0["category_scores"];

        // Map the OpenAI category names we care about into our axis
        // names. Unknown categories are ignored; missing categories
        // default to 0.
        auto get = [&](const char* key) -> double {
            if (!scores.contains(key))
                return 0.0;
            const auto& v = scores[key];
            if (v.is_number())
                return std::clamp(v.get<double>(), 0.0, 1.0);
            return 0.0;
        };

        // Max-combine related keys so we don't lose signal. E.g., both
        // "sexual/minors" and "sexual" may fire; we pick the larger for
        // the sexual axis and hold sexual_minors separately because it
        // carries a catastrophic weight in HeatConfig.
        out.sexual = std::max(get("sexual"), get("sexual/adult"));
        out.sexual_minors = get("sexual/minors");
        out.hate = std::max(get("hate"), get("hate/threatening"));
        out.violence = std::max(get("violence"), get("violence/graphic"));
        out.self_harm = std::max({get("self-harm"), get("self-harm/intent"), get("self-harm/instructions")});
        // Harassment is a superset of verbal toxicity for our purposes.
        out.toxicity =
            std::max({get("harassment"), get("harassment/threatening"), get("illicit"), get("illicit/violent")});

        return true;
    }
    catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

} // namespace moderation
