#include "WsWorkerPool.h"

#include "game/GameRoom.h"
#include "metrics/MetricsRegistry.h"
#include "net/RestRouter.h"
#include "net/WebSocketFrame.h"
#include "net/WebSocketServer.h"
#include "net/ao/AOServer.h"
#include "ServerSettings.h"
#include "utils/Log.h"

#include <algorithm>
#include <chrono>

WsWorkerPool::WsWorkerPool(WebSocketServer& ws, AOServer& ao_backend, RestRouter& router, GameRoom& room,
                           const ServerSettings& cfg)
    : slot_count_(std::max(1u, std::thread::hardware_concurrency())), ws_(ws), ao_(ao_backend), router_(router),
      room_(room), cfg_(cfg) {
    slots_.resize(slot_count_);
    for (auto& s : slots_)
        s = std::make_unique<WsSlot>();
}

void WsWorkerPool::start() {
    // Wire AO2 send function to WebSocket transport via the send queue.
    ao_.set_send_func([this](uint64_t id, const std::string& data) {
        WebSocketFrame frame;
        frame.fin = true;
        frame.rsv = 0;
        frame.opcode = TEXT;
        frame.mask = false;
        frame.len = data.size();
        if (frame.len <= 125)
            frame.len_code = static_cast<uint8_t>(frame.len & 0xFF);
        else if (frame.len <= UINT16_MAX)
            frame.len_code = 126;
        else
            frame.len_code = 127;
        frame.data.assign(reinterpret_cast<const uint8_t*>(data.data()),
                          reinterpret_cast<const uint8_t*>(data.data()) + data.size());
        ws_.queue_send(id, frame.serialize());
    });

    // Spawn per-slot worker threads
    for (int i = 0; i < slot_count_; ++i) {
        workers_.emplace_back([this, slot_idx = i](std::stop_token st) {
            auto& slot = *slots_[slot_idx];
            while (!st.stop_requested()) {
                auto idle_start = std::chrono::steady_clock::now();
                WsWorkItem item;
                {
                    std::unique_lock lock(slot.mutex);
                    slot.cv.wait(lock, [&] { return !slot.queue.empty() || st.stop_requested(); });
                    if (st.stop_requested())
                        break;
                    item = std::move(slot.queue.front());
                    slot.queue.pop_front();
                }
                auto idle_end = std::chrono::steady_clock::now();
                worker_stats_.idle_ns.fetch_add(
                    static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(idle_end - idle_start).count()),
                    std::memory_order_relaxed);

                worker_stats_.active.fetch_add(1, std::memory_order_relaxed);
                auto busy_start = std::chrono::steady_clock::now();

                router_.with_lock([&] { ao_.on_client_message(item.client_id, item.data); });
                poll_stats_.frames_dispatched.fetch_add(1, std::memory_order_relaxed);

                worker_stats_.busy_ns.fetch_add(
                    static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                              std::chrono::steady_clock::now() - busy_start)
                                              .count()),
                    std::memory_order_relaxed);
                worker_stats_.active.fetch_sub(1, std::memory_order_relaxed);
            }
        });
    }

    // Poll thread: I/O dispatch, flush sends, session expiry
    poll_thread_ = std::jthread([this](std::stop_token st) {
        auto last_sweep = std::chrono::steady_clock::now();
        while (!st.stop_requested()) {
            auto idle_start = std::chrono::steady_clock::now();
            auto frames = ws_.poll(10);
            auto idle_end = std::chrono::steady_clock::now();
            poll_stats_.idle_ns.fetch_add(
                static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(idle_end - idle_start).count()),
                std::memory_order_relaxed);

            auto busy_start = std::chrono::steady_clock::now();

            // Enqueue frames to per-client worker slots
            if (!frames.empty()) {
                for (auto& [client_id, frame] : frames) {
                    int slot_idx = static_cast<int>(client_id % static_cast<uint64_t>(slot_count_));
                    auto& slot = *slots_[slot_idx];
                    {
                        std::lock_guard lock(slot.mutex);
                        slot.queue.push_back({client_id, std::string(frame.data.begin(), frame.data.end())});
                    }
                    slot.cv.notify_one();
                }
            }

            ws_.flush_sends();

            // Periodic session expiry sweep (~every 30s)
            auto now = std::chrono::steady_clock::now();
            if (now - last_sweep > std::chrono::seconds(30)) {
                std::vector<uint64_t> to_expire;
                router_.with_shared_lock(
                    [&] { to_expire = room_.find_expired_sessions(cfg_.session_ttl_seconds()); });
                if (!to_expire.empty()) {
                    Log::log_print(INFO, "Expiring %zu sessions", to_expire.size());
                    constexpr size_t BATCH_SIZE = 64;
                    for (size_t i = 0; i < to_expire.size(); i += BATCH_SIZE) {
                        size_t end = std::min(i + BATCH_SIZE, to_expire.size());
                        router_.with_lock([&] {
                            for (size_t j = i; j < end; ++j)
                                room_.destroy_session(to_expire[j]);
                            room_.broadcast_chars_taken();
                        });
                    }
                    metrics::MetricsRegistry::instance()
                        .counter("kagami_sessions_expired_total", "Sessions expired by TTL")
                        .get()
                        .inc(to_expire.size());
                }
                last_sweep = now;
            }

            poll_stats_.busy_ns.fetch_add(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                                     std::chrono::steady_clock::now() - busy_start)
                                                                     .count()),
                                          std::memory_order_relaxed);
        }
        // Wake workers so they can check stop_requested
        for (auto& slot : slots_)
            slot->cv.notify_all();
    });
}

size_t WsWorkerPool::total_queued() const {
    size_t total = 0;
    for (auto& slot : slots_) {
        std::lock_guard lock(slot->mutex);
        total += slot->queue.size();
    }
    return total;
}
