#include "MasterServerAdvertiser.h"

#include "net/Http.h"

#include <json.hpp>

static constexpr int ADVERTISE_INTERVAL_SEC = 240; // 4 minutes, matching akashi

MasterServerAdvertiser::MasterServerAdvertiser(const ServerSettings& cfg, const GameRoom& room)
    : cfg_(cfg), room_(room) {
}

MasterServerAdvertiser::~MasterServerAdvertiser() {
    stop();
}

void MasterServerAdvertiser::start() {
    if (!cfg_.advertiser_enabled())
        return;
    if (thread_.joinable())
        return;

    thread_ = std::jthread([this](std::stop_token st) { run(st); });
}

void MasterServerAdvertiser::stop() {
    if (thread_.joinable()) {
        thread_.request_stop();
        thread_.join();
    }
}

void MasterServerAdvertiser::run(std::stop_token stoken) {
    // Publish immediately on startup, then every ADVERTISE_INTERVAL_SEC.
    publish_once();

    while (!stoken.stop_requested()) {
        // Sleep in small increments so we can respond to stop quickly.
        for (int i = 0; i < ADVERTISE_INTERVAL_SEC && !stoken.stop_requested(); ++i)
            std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!stoken.stop_requested())
            publish_once();
    }
}

bool MasterServerAdvertiser::publish_once() {
    if (!cfg_.advertiser_enabled())
        return false;

    std::string url = cfg_.advertiser_url();
    if (url.empty()) {
        Log::log_print(WARNING, "Advertiser: URL is empty, skipping");
        return false;
    }

    // Split URL into scheme+host and path.
    // e.g. "https://servers.aceattorneyonline.com/servers"
    //   -> host_part = "https://servers.aceattorneyonline.com"
    //   -> path      = "/servers"
    std::string host_part;
    std::string path = "/";

    // Find the path component after scheme://host
    auto scheme_end = url.find("://");
    if (scheme_end != std::string::npos) {
        auto path_start = url.find('/', scheme_end + 3);
        if (path_start != std::string::npos) {
            host_part = url.substr(0, path_start);
            path = url.substr(path_start);
        }
        else {
            host_part = url;
        }
    }
    else {
        Log::log_print(WARNING, "Advertiser: URL missing scheme: %s", url.c_str());
        return false;
    }

    // Build JSON payload
    nlohmann::json payload;

    std::string hostname = cfg_.domain();
    if (!hostname.empty())
        payload["ip"] = hostname;

    int ws_port = cfg_.ws_port();
    if (ws_port > 0)
        payload["ws_port"] = ws_port;

    int wss_port = cfg_.wss_port();
    if (wss_port > 0)
        payload["wss_port"] = wss_port;

    payload["port"] = 27016;
    payload["players"] = static_cast<int>(room_.session_count());
    payload["name"] = cfg_.server_name();
    payload["description"] = cfg_.server_description();

    std::string body = payload.dump();

    // POST to master server
    http::Client client(host_part);
    client.set_connection_timeout(10, 0);
    client.set_read_timeout(10, 0);

    auto result = client.Post(path, body, "application/json");
    if (!result) {
        Log::log_print(WARNING, "Advertiser: failed to connect to %s (error %d)", url.c_str(),
                       static_cast<int>(result.error()));
        return false;
    }

    if (result->status != 200) {
        // Try to parse error body
        try {
            auto doc = nlohmann::json::parse(result->body);
            if (doc.contains("errors") && doc["errors"].is_array()) {
                for (auto& err : doc["errors"]) {
                    Log::log_print(WARNING, "Advertiser: %s — %s", err.value("type", "").c_str(),
                                   err.value("message", "").c_str());
                }
            }
            else {
                Log::log_print(WARNING, "Advertiser: HTTP %d from %s", result->status, url.c_str());
            }
        }
        catch (...) {
            Log::log_print(WARNING, "Advertiser: HTTP %d from %s", result->status, url.c_str());
        }
        return false;
    }

    Log::log_print(INFO, "Advertiser: successfully advertised to %s", host_part.c_str());
    return true;
}
