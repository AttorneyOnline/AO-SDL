#include "net/HttpPool.h"

#include "utils/Log.h"

#include <httplib.h>

HttpPool::HttpPool(int num_threads) {
    for (int i = 0; i < num_threads; i++)
        workers_.emplace_back(&HttpPool::worker_loop, this);
    Log::log_print(DEBUG, "HttpPool: started %d worker threads", num_threads);
}

HttpPool::~HttpPool() {
    running_.store(false, std::memory_order_release);
    work_cv_.notify_all();
    for (auto& t : workers_)
        t.join();
}

void HttpPool::get(const std::string& host, const std::string& path, HttpCallback cb) {
    pending_.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard lock(work_mutex_);
        work_queue_.push({host, path, std::move(cb)});
    }
    work_cv_.notify_one();
}

int HttpPool::poll() {
    std::queue<CompletedRequest> batch;
    {
        std::lock_guard lock(result_mutex_);
        batch.swap(result_queue_);
    }
    int count = 0;
    while (!batch.empty()) {
        auto& item = batch.front();
        item.callback(std::move(item.response));
        batch.pop();
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
            work_queue_.pop();
        }

        HttpResponse resp;
        try {
            httplib::Client cli(req.host);
            cli.set_connection_timeout(10);
            cli.set_read_timeout(15);
            auto res = cli.Get(req.path);
            if (res) {
                resp.status = res->status;
                resp.body = std::move(res->body);
            } else {
                resp.error = httplib::to_string(res.error());
            }
        } catch (const std::exception& e) {
            resp.error = e.what();
        }

        {
            std::lock_guard lock(result_mutex_);
            result_queue_.push({std::move(resp), std::move(req.callback)});
        }
        pending_.fetch_sub(1, std::memory_order_relaxed);
    }
}
