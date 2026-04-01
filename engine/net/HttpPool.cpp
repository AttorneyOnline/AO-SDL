#include "net/HttpPool.h"

#include "utils/Log.h"

#include "net/Http.h"

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
        work_queue_.clear();
    }
    work_cv_.notify_all();
    // jthread destructors auto-join
    workers_.clear();
}

void HttpPool::get(const std::string& host, const std::string& path, HttpCallback cb, HttpPriority priority) {
    pending_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard lock(work_mutex_);
        auto it = std::find_if(work_queue_.begin(), work_queue_.end(),
                               [priority](const Request& r) { return r.priority < priority; });
        work_queue_.insert(it, {host, path, std::move(cb), nullptr, priority});
    }
    work_cv_.notify_one();
}

void HttpPool::get_streaming(const std::string& host, const std::string& path, HttpChunkCallback on_chunk,
                             HttpCallback cb, HttpPriority priority) {
    pending_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard lock(work_mutex_);
        auto it = std::find_if(work_queue_.begin(), work_queue_.end(),
                               [priority](const Request& r) { return r.priority < priority; });
        work_queue_.insert(it, {host, path, std::move(cb), std::move(on_chunk), priority});
    }
    work_cv_.notify_one();
}

void HttpPool::drop_below(HttpPriority threshold) {
    std::deque<Request> dropped_requests;
    {
        std::lock_guard lock(work_mutex_);
        auto it = work_queue_.begin();
        while (it != work_queue_.end()) {
            if (it->priority < threshold) {
                dropped_requests.push_back(std::move(*it));
                it = work_queue_.erase(it);
                pending_.fetch_sub(1, std::memory_order_relaxed);
            }
            else {
                ++it;
            }
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
            work_cv_.wait(lock, [this, &st] { return !work_queue_.empty() || st.stop_requested(); });
            if (st.stop_requested() && work_queue_.empty())
                return;
            req = std::move(work_queue_.front());
            work_queue_.pop_front();
        }

        HttpResponse resp;
        if (!st.stop_requested()) {
            try {
                auto& cli = get_client(req.host);
                if (req.chunk_callback) {
                    // Streaming: longer read timeout for large files (music)
                    cli.set_read_timeout(30);
                    auto res = cli.Get(req.path, [&](const char* data, size_t len) -> bool {
                        return req.chunk_callback(reinterpret_cast<const uint8_t*>(data), len);
                    });
                    cli.set_read_timeout(10); // restore normal timeout
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
        }

        if (!st.stop_requested()) {
            if (req.chunk_callback) {
                // Streaming: callback runs directly on worker thread (caller blocks on future)
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
