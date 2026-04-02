#include "net/HttpPool.h"

#include "utils/Log.h"

#include "net/Http.h"
#include "net/Http2Connection.h"

#include <algorithm>

HttpPool::HttpPool(int num_threads) {
    for (int i = 0; i < num_threads; i++)
        workers_.emplace_back([this](std::stop_token st) { worker_loop(st); });
    Log::log_print(DEBUG, "HttpPool: started %d worker threads", num_threads);
}

HttpPool::~HttpPool() {
    stop();
}

void HttpPool::stop() {
    if (workers_.empty())
        return;
    for (auto& t : workers_)
        t.request_stop();
    {
        std::lock_guard lock(work_mutex_);
        for (auto& q : work_queues_)
            q.clear();
    }
    work_cv_.notify_all();

    Log::log_print(DEBUG, "HttpPool::stop: shutting down h2 connections");
    {
        std::lock_guard lock(h2_mutex_);
        for (auto& [host, conn] : h2_connections_) {
            Log::log_print(DEBUG, "HttpPool::stop: shutting down h2 for %s", host.c_str());
            conn->shutdown();
        }
        h2_connections_.clear();
        h2_failed_hosts_.clear();
        h2_eligible_hosts_.clear();
        h2_connecting_hosts_.clear();
    }

    Log::log_print(DEBUG, "HttpPool::stop: joining workers");
    workers_.clear();
    Log::log_print(DEBUG, "HttpPool::stop: done");
}

void HttpPool::mark_h2_eligible(const std::string& host) {
    std::lock_guard lock(h2_mutex_);
    if (!h2_failed_hosts_.count(host))
        h2_eligible_hosts_.insert(host);
}

std::shared_ptr<Http2Connection> HttpPool::get_h2(const std::string& host) {
    {
        std::lock_guard lock(h2_mutex_);
        if (h2_failed_hosts_.count(host))
            return nullptr;

        auto it = h2_connections_.find(host);
        if (it != h2_connections_.end()) {
            if (it->second->is_alive())
                return it->second;
            h2_connections_.erase(it);
        }

        if (!h2_eligible_hosts_.count(host)) {
            Log::log_print(DEBUG, "HttpPool: h2 skip (not eligible yet): %s", host.c_str());
            return nullptr;
        }

        // Prevent multiple workers from racing to connect to the same host.
        // Only the first worker connects; others fall back to h1.
        if (h2_connecting_hosts_.count(host))
            return nullptr;
        h2_connecting_hosts_.insert(host);
    }

    // Parse host:port from the URL-style host string
    auto parsed_host = host;
    uint16_t port = 443;
    auto scheme_end = parsed_host.find("://");
    if (scheme_end != std::string::npos)
        parsed_host = parsed_host.substr(scheme_end + 3);
    if (host.find("http://") == 0) {
        std::lock_guard lock(h2_mutex_);
        h2_failed_hosts_.insert(host);
        h2_connecting_hosts_.erase(host);
        return nullptr;
    }
    auto slash = parsed_host.find('/');
    if (slash != std::string::npos)
        parsed_host = parsed_host.substr(0, slash);
    auto colon = parsed_host.find(':');
    if (colon != std::string::npos) {
        port = static_cast<uint16_t>(std::stoi(parsed_host.substr(colon + 1)));
        parsed_host = parsed_host.substr(0, colon);
    }

    try {
        auto conn = Http2Connection::connect(parsed_host, port, 3000);
        std::lock_guard lock(h2_mutex_);
        h2_connecting_hosts_.erase(host);
        if (!conn) {
            h2_failed_hosts_.insert(host);
            Log::log_print(DEBUG, "HttpPool: %s does not support HTTP/2, using HTTP/1.1", host.c_str());
            return nullptr;
        }
        Log::log_print(DEBUG, "HttpPool: HTTP/2 connection established to %s", host.c_str());
        auto ptr = std::shared_ptr<Http2Connection>(std::move(conn));
        h2_connections_[host] = ptr;
        return ptr;
    }
    catch (const std::exception& e) {
        std::lock_guard lock(h2_mutex_);
        h2_connecting_hosts_.erase(host);
        h2_failed_hosts_.insert(host);
        Log::log_print(DEBUG, "HttpPool: HTTP/2 connect to %s failed: %s", host.c_str(), e.what());
        return nullptr;
    }
}

void HttpPool::get(const std::string& host, const std::string& path, HttpCallback cb, HttpPriority priority) {
    pending_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard lock(work_mutex_);
        int idx = static_cast<int>(priority);
        work_queues_[idx].push_back({host, path, std::move(cb), nullptr, priority});
    }
    work_cv_.notify_one();
}

void HttpPool::get_streaming(const std::string& host, const std::string& path, HttpChunkCallback on_chunk,
                             HttpCallback cb, HttpPriority priority) {
    pending_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard lock(work_mutex_);
        int idx = static_cast<int>(priority);
        work_queues_[idx].push_back({host, path, std::move(cb), std::move(on_chunk), priority});
    }
    work_cv_.notify_one();
}

void HttpPool::drop_below(HttpPriority threshold) {
    std::deque<Request> dropped_requests;
    {
        std::lock_guard lock(work_mutex_);
        // Iterate from highest priority down so callbacks fire in priority order
        for (int i = static_cast<int>(threshold) - 1; i >= 0; --i) {
            auto& q = work_queues_[i];
            pending_.fetch_sub(static_cast<int>(q.size()), std::memory_order_relaxed);
            for (auto& req : q)
                dropped_requests.push_back(std::move(req));
            q.clear();
        }
    }
    // Fire callbacks with empty response so callers can clean up (e.g. MountHttp::pending_)
    for (auto& req : dropped_requests) {
        if (req.callback) {
            HttpResponse resp;
            resp.status = 0;
            resp.error = "dropped";
            req.callback(std::move(resp));
        }
    }
    int dropped = (int)dropped_requests.size();
    if (dropped > 0)
        Log::log_print(DEBUG, "HttpPool: dropped %d low-priority requests", dropped);
}

int HttpPool::poll() {
    std::deque<CompletedRequest> batch;
    {
        std::lock_guard lock(result_mutex_);
        batch.swap(result_queue_);
    }
    int count = 0;
    while (!batch.empty()) {
        auto& item = batch.front();
        item.callback(std::move(item.response));
        batch.pop_front();
        count++;
    }
    return count;
}

bool HttpPool::pop_highest(Request& out) {
    // Scan from highest priority (CRITICAL=3) down to lowest (LOW=0)
    for (int i = NUM_PRIORITIES - 1; i >= 0; --i) {
        if (!work_queues_[i].empty()) {
            out = std::move(work_queues_[i].front());
            work_queues_[i].pop_front();
            return true;
        }
    }
    return false;
}

void HttpPool::worker_loop(std::stop_token st) {
    // Keep one persistent http::Client per host so TCP+SSL connections are
    // reused via HTTP keep-alive.  This avoids creating a new SSL_CTX, loading
    // the Windows certificate store, and performing a full TLS handshake for
    // every single request.
    std::unordered_map<std::string, std::unique_ptr<http::Client>> clients;

    auto get_client = [&](const std::string& host) -> http::Client& {
        auto it = clients.find(host);
        if (it != clients.end())
            return *it->second;
        auto cli = std::make_unique<http::Client>(host);
        cli->set_connection_timeout(5);
        cli->set_read_timeout(10);
        cli->set_keep_alive(true);
        auto& ref = *cli;
        clients.emplace(host, std::move(cli));
        return ref;
    };

    while (true) {
        Request req;
        {
            std::unique_lock lock(work_mutex_);
            work_cv_.wait(lock, [this, &st] {
                if (st.stop_requested())
                    return true;
                for (auto& q : work_queues_)
                    if (!q.empty())
                        return true;
                return false;
            });
            if (st.stop_requested() && !pop_highest(req))
                return;
            if (!pop_highest(req))
                continue;
        }

        HttpResponse resp;
        bool done = false;
        if (!st.stop_requested()) {

            // Try HTTP/2 first for non-streaming requests.
            // Use async submit so the worker doesn't block — the h2 I/O
            // thread delivers results directly to result_queue_.
            if (!req.chunk_callback) {
                auto h2 = get_h2(req.host);
                if (h2) {
                    static constexpr int priority_to_urgency[] = {6, 3, 1, 0};
                    int urgency = priority_to_urgency[static_cast<int>(req.priority)];
                    auto cb = std::move(req.callback);
                    h2->submit_get_async(req.path, urgency,
                                         [this, cb = std::move(cb)](Http2Connection::Response h2_resp) {
                                             HttpResponse resp;
                                             resp.status = h2_resp.status;
                                             resp.body = std::move(h2_resp.body);
                                             resp.error = std::move(h2_resp.error);
                                             if (cb) {
                                                 std::lock_guard lock(result_mutex_);
                                                 result_queue_.push_back({std::move(resp), cb});
                                             }
                                             pending_.fetch_sub(1, std::memory_order_relaxed);
                                         });
                    done = true;
                    // Don't decrement pending here — the async callback does it
                }
            }

            // Fallback to HTTP/1.1
            if (!done) {
                try {
                    auto& cli = get_client(req.host);
                    if (req.chunk_callback) {
                        cli.set_read_timeout(30);
                        auto res = cli.Get(req.path, [&](const char* data, size_t len) -> bool {
                            return req.chunk_callback(reinterpret_cast<const uint8_t*>(data), len);
                        });
                        cli.set_read_timeout(10);
                        if (res) {
                            resp.status = res->status;
                        }
                        else {
                            resp.error = http::to_string(res.error());
                            clients.erase(req.host);
                        }
                    }
                    else {
                        auto res = cli.Get(req.path);
                        if (res) {
                            resp.status = res->status;
                            resp.body = std::move(res->body);
                        }
                        else {
                            resp.error = http::to_string(res.error());
                            clients.erase(req.host);
                        }
                    }
                }
                catch (const std::exception& e) {
                    resp.error = e.what();
                    clients.erase(req.host);
                }
                // After a successful h1 request, mark host as eligible for h2 upgrade
                if (resp.status > 0)
                    mark_h2_eligible(req.host);
            }
        }

        if (done) {
            // h2 async path — callback handles result queue and pending count
            continue;
        }

        if (!st.stop_requested()) {
            if (req.chunk_callback) {
                if (req.callback)
                    req.callback(std::move(resp));
            }
            else {
                std::lock_guard lock(result_mutex_);
                result_queue_.push_back({std::move(resp), std::move(req.callback)});
            }
        }
        pending_.fetch_sub(1, std::memory_order_relaxed);
    }
}
