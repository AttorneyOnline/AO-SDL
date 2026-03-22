#include "net/HttpPool.h"

#include "utils/Log.h"

#include <httplib.h>

#include <algorithm>

HttpPool::HttpPool(int num_threads) {
    for (int i = 0; i < num_threads; i++)
        workers_.emplace_back(&HttpPool::worker_loop, this);
    Log::log_print(DEBUG, "HttpPool: started %d worker threads", num_threads);
}

HttpPool::~HttpPool() {
    running_.store(false, std::memory_order_release);
    {
        std::lock_guard lock(work_mutex_);
        work_queue_.clear();
    }
    work_cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable())
            t.detach();
    }
}

void HttpPool::get(const std::string& host, const std::string& path, HttpCallback cb, HttpPriority priority) {
    pending_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard lock(work_mutex_);
        // Insert sorted by priority (highest first). Find the first element
        // with lower priority and insert before it.
        auto it = std::find_if(work_queue_.begin(), work_queue_.end(),
                               [priority](const Request& r) { return r.priority < priority; });
        work_queue_.insert(it, {host, path, std::move(cb), priority});
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

void HttpPool::worker_loop() {
    while (true) {
        Request req;
        {
            std::unique_lock lock(work_mutex_);
            work_cv_.wait(lock, [this] { return !work_queue_.empty() || !running_.load(std::memory_order_acquire); });
            if (!running_.load(std::memory_order_acquire) && work_queue_.empty())
                return;
            req = std::move(work_queue_.front());
            work_queue_.pop_front();
        }

        HttpResponse resp;
        if (running_.load(std::memory_order_acquire)) {
            try {
                httplib::Client cli(req.host);
                cli.set_connection_timeout(5);
                cli.set_read_timeout(10);
                auto res = cli.Get(req.path);
                if (res) {
                    resp.status = res->status;
                    resp.body = std::move(res->body);
                }
                else {
                    resp.error = httplib::to_string(res.error());
                }
            }
            catch (const std::exception& e) {
                resp.error = e.what();
            }
        }

        if (running_.load(std::memory_order_acquire)) {
            std::lock_guard lock(result_mutex_);
            result_queue_.push_back({std::move(resp), std::move(req.callback)});
        }
        pending_.fetch_sub(1, std::memory_order_relaxed);
    }
}
