#include "MetricsCollector.h"
#include "ServerSettings.h"
#include "WsWorkerPool.h"

#include "event/EventManager.h"
#include "game/GameRoom.h"
#include "metrics/MetricsRegistry.h"
#include "metrics/ProcessMetrics.h"
#include "net/Http.h"
#include "net/RestRouter.h"
#include "net/WebSocketServer.h"
#include "utils/Version.h"

MetricsCollector::MetricsCollector(GameRoom& room, RestRouter& router, const ServerSettings& cfg,
                                   std::chrono::steady_clock::time_point start_time)
    : cache_(std::make_shared<MetricsTextCache>()), room_(room), router_(router), cfg_(cfg),
      start_time_(start_time) {}

void MetricsCollector::start(http::Server& http) {
    if (!cfg_.metrics_enabled())
        return;

    // Register /metrics HTTP handler (serves cached text, near-zero cost)
    http.GetInline(cfg_.metrics_path(), [cache = cache_](const http::Request&, http::Response& res) {
        cache->request_refresh();
        auto text = cache->load();
        res.set_content(*text, "text/plain; version=0.0.4; charset=utf-8");
    });

    // Register all gauges
    auto& reg = metrics::MetricsRegistry::instance();
    auto& uptime = reg.gauge("kagami_uptime_seconds", "Server uptime in seconds");
    auto& rss = reg.gauge("kagami_process_resident_bytes", "Resident memory in bytes");
    auto& sessions_g = reg.gauge("kagami_sessions", "Active sessions", {"protocol"});
    auto& sessions_joined = reg.gauge("kagami_sessions_joined", "Joined sessions");
    auto& sessions_mods = reg.gauge("kagami_sessions_moderators", "Online moderators");
    auto& area_players = reg.gauge("kagami_area_players", "Players per area", {"area", "status"});
    auto& chars_taken = reg.gauge("kagami_characters_taken", "Characters currently taken");
    auto& area_info = reg.gauge("kagami_area_info", "Area state", {"area", "status", "locked"});
    auto& server_info = reg.gauge("kagami_server_info", "Server build and configuration",
                                  {"version", "server_name", "max_players", "session_ttl", "http_port", "ws_port",
                                   "bind_address", "cors_origin"});
    auto& max_players = reg.gauge("kagami_max_players", "Configured max player slots");
    auto& event_publishes =
        reg.gauge("kagami_event_publishes", "Cumulative events published per channel", {"channel"});
    auto& session_bytes_sent = reg.gauge("kagami_session_bytes_sent", "Bytes sent per session",
                                         {"session_id", "display_name", "protocol", "area", "character"});
    auto& session_bytes_recv = reg.gauge("kagami_session_bytes_received", "Bytes received per session",
                                         {"session_id", "display_name", "protocol", "area", "character"});
    auto& session_packets_sent = reg.gauge("kagami_session_packets_sent", "Packets sent per session",
                                           {"session_id", "display_name", "protocol", "area", "character"});
    auto& session_packets_recv = reg.gauge("kagami_session_packets_received", "Packets received per session",
                                           {"session_id", "display_name", "protocol", "area", "character"});
    auto& session_idle = reg.gauge("kagami_session_idle_seconds", "Seconds since last activity",
                                   {"session_id", "display_name", "protocol", "area", "character"});
    auto& http_open_conns = reg.gauge("kagami_http_open_connections", "Currently open TCP connections");
    auto& http_work_queue = reg.gauge("kagami_http_work_queue_depth", "Pending requests in worker queue");
    auto& http_result_queue = reg.gauge("kagami_http_result_queue_depth", "Pending results awaiting poll thread");
    auto& http_active_workers =
        reg.gauge("kagami_http_active_workers", "Worker threads currently executing handlers");
    auto& http_worker_count = reg.gauge("kagami_http_worker_count", "Total worker threads");
    auto& http_worker_util =
        reg.gauge("kagami_http_worker_utilization", "Worker pool utilization (0-1, avg across workers)");
    auto& http_worker_util_per =
        reg.gauge("kagami_http_worker_utilization_per_worker", "Per-worker utilization (0-1)", {"worker"});
    auto& cow_copy_bytes =
        reg.gauge("kagami_cow_copy_bytes_total", "Cumulative bytes copied during COW session map mutations");
    auto& poll_util = reg.gauge("kagami_http_poll_utilization", "Poll thread utilization (0-1)");
    auto& poll_events = reg.gauge("kagami_http_poll_events_total", "Total events processed by poll thread");
    auto& poll_section_ns = reg.gauge("kagami_http_poll_section_nanoseconds_total",
                                      "Cumulative poll thread time per section in nanoseconds", {"section"});
    auto& worker_section_ns = reg.gauge("kagami_http_worker_section_nanoseconds_total",
                                        "Cumulative worker time per worker per section", {"worker", "section"});
    auto& io_uring_stats = reg.gauge("kagami_io_uring_ops_total", "io_uring operation counters", {"server", "op"});
    auto& ws_poll_util = reg.gauge("kagami_ws_poll_utilization", "WebSocket poll thread utilization (0-1)");
    auto& ws_dispatch_rate = reg.gauge("kagami_ws_dispatch_rate", "WebSocket frames dispatched per second");
    auto& ws_worker_util = reg.gauge("kagami_ws_worker_utilization", "WS worker pool utilization (0-1)");
    auto& ws_worker_active = reg.gauge("kagami_ws_active_workers", "WS workers currently executing handlers");
    auto& ws_work_queue_depth =
        reg.gauge("kagami_ws_work_queue_depth", "Pending WS frames awaiting worker dispatch");
    auto& lock_util = reg.gauge("kagami_dispatch_lock", "Dispatch mutex stats", {"type"});

    auto cors = cfg_.cors_origins();
    server_info
        .labels({ao_sdl_version(), cfg_.server_name(), std::to_string(cfg_.max_players()),
                 std::to_string(cfg_.session_ttl_seconds()), std::to_string(cfg_.http_port()),
                 std::to_string(cfg_.ws_port()), cfg_.bind_address(), cors.empty() ? "" : cors[0]})
        .set(1);
    max_players.get().set(cfg_.max_players());

    // Spawn the collection thread (captures &http — http::Server outlives MetricsCollector)
    thread_ = std::jthread([this, &http, &uptime, &rss, &sessions_g, &sessions_joined, &sessions_mods, &area_players,
                            &area_info, &chars_taken, &event_publishes, &session_bytes_sent, &session_bytes_recv,
                            &session_packets_sent, &session_packets_recv, &session_idle, &http_open_conns,
                            &http_work_queue, &http_result_queue, &http_active_workers, &http_worker_count,
                            &http_worker_util, &http_worker_util_per, &cow_copy_bytes, &poll_util, &poll_events,
                            &poll_section_ns, &worker_section_ns, &io_uring_stats, &ws_poll_util, &ws_dispatch_rate,
                            &ws_worker_util, &ws_worker_active, &ws_work_queue_depth, &lock_util,
                            &reg](std::stop_token st) {
        uint64_t prev_worker_busy = 0, prev_worker_idle = 0;
        uint64_t prev_poll_busy = 0, prev_poll_idle = 0;
        size_t worker_count = 0;
        std::vector<uint64_t> prev_pw_busy;
        std::vector<uint64_t> prev_pw_idle;

        uint64_t prev_ws_busy = 0, prev_ws_idle = 0;
        uint64_t prev_ws_dispatched = 0;
        auto prev_ws_time = std::chrono::steady_clock::now();

        uint64_t prev_ws_wk_busy = 0, prev_ws_wk_idle = 0;

        uint64_t prev_excl_acq = 0, prev_excl_wait = 0, prev_excl_hold = 0;
        uint64_t prev_shared_acq = 0, prev_shared_wait = 0;
        auto prev_lock_time = std::chrono::steady_clock::now();

        while (!st.stop_requested()) {
            auto now = std::chrono::steady_clock::now();
            uptime.get().set(std::chrono::duration<double>(now - start_time_).count());
            rss.get().set(static_cast<double>(metrics::process_rss_bytes()));

            // HTTP server metrics
            http_open_conns.get().set(static_cast<double>(http.open_connections()));
            http_work_queue.get().set(static_cast<double>(http.work_queue_depth()));
            http_result_queue.get().set(static_cast<double>(http.result_queue_depth()));
            http_active_workers.get().set(static_cast<double>(http.active_workers()));
            http_worker_count.get().set(static_cast<double>(http.worker_count()));

            // Worker utilization (delta-based)
            {
                auto cur_busy = http.worker_busy_ns();
                auto cur_idle = http.worker_idle_ns();
                auto d_busy = cur_busy - prev_worker_busy;
                auto d_idle = cur_idle - prev_worker_idle;
                auto d_total = d_busy + d_idle;
                http_worker_util.get().set(d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0);
                prev_worker_busy = cur_busy;
                prev_worker_idle = cur_idle;
            }

            // Per-worker utilization (lazy init)
            if (worker_count == 0) {
                worker_count = http.worker_count();
                prev_pw_busy.resize(worker_count, 0);
                prev_pw_idle.resize(worker_count, 0);
            }
            for (size_t w = 0; w < worker_count; ++w) {
                auto cur_idle = http.worker_section_ns(w, 0);
                uint64_t cur_busy = 0;
                for (size_t s = 1; s < http.worker_section_count(); ++s)
                    cur_busy += http.worker_section_ns(w, s);
                auto d_busy = cur_busy - prev_pw_busy[w];
                auto d_idle = cur_idle - prev_pw_idle[w];
                auto d_total = d_busy + d_idle;
                http_worker_util_per.labels({std::to_string(w)})
                    .set(d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0);
                prev_pw_busy[w] = cur_busy;
                prev_pw_idle[w] = cur_idle;
            }

            // Session stats
            sessions_g.labels({"ao2"}).set(room_.stats.sessions_ao.load(std::memory_order_relaxed));
            sessions_g.labels({"aonx"}).set(room_.stats.sessions_nx.load(std::memory_order_relaxed));
            sessions_joined.get().set(room_.stats.joined.load(std::memory_order_relaxed));
            sessions_mods.get().set(room_.stats.moderators.load(std::memory_order_relaxed));
            chars_taken.get().set(room_.stats.chars_taken.load(std::memory_order_relaxed));
            cow_copy_bytes.get().set(static_cast<double>(room_.cow_copy_bytes()));

            // Poll thread utilization (delta-based)
            {
                auto cur_busy = http.poll_busy_ns();
                auto cur_idle = http.poll_idle_ns();
                auto d_busy = cur_busy - prev_poll_busy;
                auto d_idle = cur_idle - prev_poll_idle;
                auto d_total = d_busy + d_idle;
                poll_util.get().set(d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0);
                prev_poll_busy = cur_busy;
                prev_poll_idle = cur_idle;
            }
            poll_events.get().set(static_cast<double>(http.poll_events_total()));
            for (size_t i = 0; i < http.poll_section_count(); ++i)
                poll_section_ns.labels({http.poll_section_name(i)})
                    .set(static_cast<double>(http.poll_section_ns(i)));

            // io_uring stats
            auto emit_io_stats = [&](const std::string& server, const platform::Poller::IoStats& ios) {
                io_uring_stats.labels({server, "recv_submitted"}).set(static_cast<double>(ios.recv_submitted));
                io_uring_stats.labels({server, "recv_completed"}).set(static_cast<double>(ios.recv_completed));
                io_uring_stats.labels({server, "recv_enobufs"}).set(static_cast<double>(ios.recv_enobufs));
                io_uring_stats.labels({server, "send_submitted"}).set(static_cast<double>(ios.send_submitted));
                io_uring_stats.labels({server, "send_completed"}).set(static_cast<double>(ios.send_completed));
                io_uring_stats.labels({server, "send_partial"}).set(static_cast<double>(ios.send_partial));
                io_uring_stats.labels({server, "send_errors"}).set(static_cast<double>(ios.send_errors));
                io_uring_stats.labels({server, "sqe_full"}).set(static_cast<double>(ios.sqe_full));
                io_uring_stats.labels({server, "cqe_reaped"}).set(static_cast<double>(ios.cqe_reaped));
            };
            emit_io_stats("http", http.io_stats());
            if (auto* wsp = ws_ptr_.load(std::memory_order_acquire))
                emit_io_stats("ws", wsp->io_stats());

            // WS poll thread utilization (delta-based)
            if (ws_pool_) {
                auto& wps = ws_pool_->poll_stats();
                auto cur_busy = wps.busy_ns.load(std::memory_order_relaxed);
                auto cur_idle = wps.idle_ns.load(std::memory_order_relaxed);
                auto cur_disp = wps.frames_dispatched.load(std::memory_order_relaxed);
                auto cur_time = std::chrono::steady_clock::now();

                auto d_busy = cur_busy - prev_ws_busy;
                auto d_idle = cur_idle - prev_ws_idle;
                auto d_total = d_busy + d_idle;
                ws_poll_util.get().set(d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0);

                double dt = std::chrono::duration<double>(cur_time - prev_ws_time).count();
                ws_dispatch_rate.get().set(dt > 0 ? (cur_disp - prev_ws_dispatched) / dt : 0.0);

                prev_ws_busy = cur_busy;
                prev_ws_idle = cur_idle;
                prev_ws_dispatched = cur_disp;
                prev_ws_time = cur_time;
            }

            // WS worker pool utilization (delta-based)
            if (ws_pool_) {
                auto& wws = ws_pool_->worker_stats();
                auto cur_busy = wws.busy_ns.load(std::memory_order_relaxed);
                auto cur_idle = wws.idle_ns.load(std::memory_order_relaxed);
                auto d_busy = cur_busy - prev_ws_wk_busy;
                auto d_idle = cur_idle - prev_ws_wk_idle;
                auto d_total = d_busy + d_idle;
                ws_worker_util.get().set(d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0);
                prev_ws_wk_busy = cur_busy;
                prev_ws_wk_idle = cur_idle;

                ws_worker_active.get().set(wws.active.load(std::memory_order_relaxed));
                ws_work_queue_depth.get().set(static_cast<double>(ws_pool_->total_queued()));
            }

            // Dispatch lock stats (delta-based)
            {
                auto& ls = router_.lock_stats;
                auto cur_excl_acq = ls.exclusive_acquisitions.load(std::memory_order_relaxed);
                auto cur_excl_wait = ls.exclusive_wait_ns.load(std::memory_order_relaxed);
                auto cur_excl_hold = ls.exclusive_hold_ns.load(std::memory_order_relaxed);
                auto cur_shared_acq = ls.shared_acquisitions.load(std::memory_order_relaxed);
                auto cur_shared_wait = ls.shared_wait_ns.load(std::memory_order_relaxed);

                auto d_excl_acq = cur_excl_acq - prev_excl_acq;
                auto d_excl_wait = cur_excl_wait - prev_excl_wait;
                auto d_excl_hold = cur_excl_hold - prev_excl_hold;
                auto d_shared_acq = cur_shared_acq - prev_shared_acq;
                auto d_shared_wait = cur_shared_wait - prev_shared_wait;

                auto cur_lock_time = std::chrono::steady_clock::now();
                double lock_dt = std::chrono::duration<double>(cur_lock_time - prev_lock_time).count();

                lock_util.labels({"exclusive_acquisitions_per_sec"}).set(lock_dt > 0 ? d_excl_acq / lock_dt : 0.0);
                lock_util.labels({"shared_acquisitions_per_sec"}).set(lock_dt > 0 ? d_shared_acq / lock_dt : 0.0);
                lock_util.labels({"exclusive_avg_wait_ns"})
                    .set(d_excl_acq > 0 ? static_cast<double>(d_excl_wait) / d_excl_acq : 0.0);
                lock_util.labels({"shared_avg_wait_ns"})
                    .set(d_shared_acq > 0 ? static_cast<double>(d_shared_wait) / d_shared_acq : 0.0);
                lock_util.labels({"exclusive_hold_ns_per_sec"}).set(lock_dt > 0 ? d_excl_hold / lock_dt : 0.0);

                prev_lock_time = cur_lock_time;
                prev_excl_acq = cur_excl_acq;
                prev_excl_wait = cur_excl_wait;
                prev_excl_hold = cur_excl_hold;
                prev_shared_acq = cur_shared_acq;
                prev_shared_wait = cur_shared_wait;
            }

            // Per-worker section breakdown
            for (size_t w = 0; w < worker_count; ++w)
                for (size_t s = 0; s < http.worker_section_count(); ++s)
                    worker_section_ns.labels({std::to_string(w), http.worker_section_name(s)})
                        .set(static_cast<double>(http.worker_section_ns(w, s)));

            // Per-session + area detail
            auto snap = room_.sessions_snapshot();

            area_players.clear();
            area_info.clear();
            for (auto& [id, state] : room_.area_states()) {
                int count = 0;
                snap.sessions.for_each([&](const uint64_t&, const GameRoom::SessionPtr& s) {
                    if (s->area == state.name)
                        ++count;
                });
                area_players.labels({state.name, state.status}).set(count);
                area_info.labels({state.name, state.status, state.locked ? "true" : "false"}).set(1);
            }

            session_bytes_sent.clear();
            session_bytes_recv.clear();
            session_packets_sent.clear();
            session_packets_recv.clear();
            session_idle.clear();

            snap.sessions.for_each([&](const uint64_t&, const GameRoom::SessionPtr& s) {
                std::string char_name = (s->character_id >= 0 && s->character_id < (int)room_.characters.size())
                                            ? room_.characters[s->character_id]
                                            : "none";
                std::vector<std::string> labels = {std::to_string(s->session_id), s->display_name, s->protocol,
                                                   s->area, std::move(char_name)};
                session_bytes_sent.labels(labels).set(
                    static_cast<double>(s->bytes_sent.load(std::memory_order_relaxed)));
                session_bytes_recv.labels(labels).set(
                    static_cast<double>(s->bytes_received.load(std::memory_order_relaxed)));
                session_packets_sent.labels(labels).set(
                    static_cast<double>(s->packets_sent.load(std::memory_order_relaxed)));
                session_packets_recv.labels(labels).set(
                    static_cast<double>(s->packets_received.load(std::memory_order_relaxed)));
                session_idle.labels(labels).set(static_cast<double>(
                    std::chrono::duration_cast<std::chrono::seconds>(now - s->last_activity()).count()));
            });

            for (auto& cs : EventManager::instance().snapshot_channel_stats())
                event_publishes.labels({cs.raw_name}).set(static_cast<double>(cs.count));

            // Serialize and cache
            auto text = std::make_shared<const std::string>(reg.collect());
            cache_->store(std::move(text));

            cache_->wait_for_request(st, std::chrono::seconds(2));
        }
    });
}
