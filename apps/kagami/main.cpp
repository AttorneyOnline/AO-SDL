#include "CloudWatchSink.h"
#include "LokiSink.h"
#include "ReplCommand.h"
#include "ServerSettings.h"
#include "TerminalUI.h"

#include "event/EventManager.h"
#include "game/ClientId.h"
#include "game/GameRoom.h"
#include "metrics/MetricsRegistry.h"
#include "metrics/ProcessMetrics.h"
#include "net/EndpointFactory.h"
#include "net/PlatformServerSocket.h"
#include "net/RestRouter.h"
#include "net/WebSocketServer.h"
#include "net/ao/AOServer.h"
#include "net/nx/NXEndpoint.h"
#include "utils/Log.h"
#include "utils/Version.h"

#include "net/Http.h"

#include <condition_variable>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <io.h>
#define IS_INTERACTIVE() (_isatty(_fileno(stdin)))
#else
#include <unistd.h>
#define IS_INTERACTIVE() (isatty(fileno(stdin)))
#endif

static std::stop_source stop_src;

static void signal_handler(int) {
    stop_src.request_stop();
}

/// Resolve the path to kagami.json next to the binary.
static std::string config_path(const char* argv0) {
    auto bin_dir = std::filesystem::path(argv0).parent_path();
    return (bin_dir / "kagami.json").string();
}

int main(int /*argc*/, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // --- Load configuration ---
    std::string cfg_path = config_path(argv[0]);
    ServerSettings::load_from_disk(cfg_path);

    auto& cfg = ServerSettings::instance();

    // --- Terminal UI (interactive mode only) ---
    bool interactive = IS_INTERACTIVE();
    TerminalUI ui;

    // --- Log level for stdout (and terminal UI in interactive mode) ---
    Log::set_stdout_level(cfg.console_log_level());

    if (interactive) {
        ui.init();
        Log::set_sink([&ui](LogLevel level, const std::string& timestamp,
                            const std::string& message) { ui.log(level, timestamp, message); },
                      cfg.console_log_level());
    }

    // --- File log sink ---
    std::shared_ptr<std::ofstream> log_file;
    if (!cfg.log_file().empty()) {
        log_file = std::make_shared<std::ofstream>(cfg.log_file(), std::ios::app);
        if (log_file->is_open()) {
            Log::add_sink(
                "file",
                [lf = log_file](LogLevel level, const std::string& timestamp, const std::string& message) {
                    *lf << "[" << timestamp << "][" << log_level_name(level) << "] " << message << "\n";
                    lf->flush();
                },
                cfg.file_log_level());
        }
        else {
            Log::log_print(WARNING, "Could not open log file: %s", cfg.log_file().c_str());
            log_file.reset();
        }
    }

    // --- CloudWatch log sink ---
    std::unique_ptr<CloudWatchSink> cw_sink;
    if (!cfg.cloudwatch_region().empty() && !cfg.cloudwatch_log_group().empty()) {
        CloudWatchSink::Config cw_cfg;
        cw_cfg.region = cfg.cloudwatch_region();
        cw_cfg.log_group = cfg.cloudwatch_log_group();
        cw_cfg.log_stream = cfg.cloudwatch_log_stream();
        cw_cfg.credentials.access_key_id = cfg.cloudwatch_access_key_id();
        cw_cfg.credentials.secret_access_key = cfg.cloudwatch_secret_access_key();
        cw_cfg.flush_interval_seconds = cfg.cloudwatch_flush_interval();

        if (cw_cfg.log_stream.empty())
            cw_cfg.log_stream = cfg.server_name();

        Log::log_print(INFO, "CloudWatch: group=%s stream=%s region=%s", cw_cfg.log_group.c_str(),
                       cw_cfg.log_stream.c_str(), cw_cfg.region.c_str());

        cw_sink = std::make_unique<CloudWatchSink>(std::move(cw_cfg));
        Log::add_sink(
            "cloudwatch",
            [&cw = *cw_sink](LogLevel level, const std::string& timestamp, const std::string& message) {
                cw.push(level, timestamp, message);
            },
            cfg.cloudwatch_log_level());
        cw_sink->start();
    }

    // --- Loki log sink ---
    std::unique_ptr<LokiSink> loki_sink;
    if (!cfg.loki_url().empty()) {
        LokiSink::Config loki_cfg;
        loki_cfg.url = cfg.loki_url();
        loki_sink = std::make_unique<LokiSink>(std::move(loki_cfg));
        Log::add_sink(
            "loki",
            [&lk = *loki_sink](LogLevel level, const std::string& timestamp, const std::string& message) {
                lk.push(level, timestamp, message);
            },
            cfg.console_log_level());
        loki_sink->start();
        Log::log_print(INFO, "Loki: pushing to %s", cfg.loki_url().c_str());
    }

    Log::log_print(INFO, "Server: %s", cfg.server_name().c_str());

    // --- Shared game state ---
    GameRoom room;
    room.server_name = cfg.server_name();
    room.server_description = cfg.server_description();
    room.max_players = cfg.max_players();
    room.characters = {"Phoenix", "Edgeworth", "Maya", "Godot", "Apollo"};
    room.music = {"Trial.opus", "Objection.opus", "Pursuit.opus"};
    room.areas = {"Lobby", "Courtroom 1", "Courtroom 2"};
    room.reset_taken();
    room.build_char_id_index();
    room.build_area_index();

    // --- Protocol backends ---
    AOServer ao_backend(room); // AO2: WebSocket bidirectional
    NXServer nx_backend(room); // AONX: REST + SSE (no WebSocket)
    nx_backend.set_motd(cfg.motd());
    nx_backend.set_session_ttl_seconds(cfg.session_ttl_seconds());
    // --- HTTP server (runs on its own thread pool) ---
    http::Server http;

    http.Get("/", [&](const http::Request&, http::Response& res) {
        res.set_content("Hello from " + cfg.server_name() + "\n", "text/plain");
    });

    // --- Metrics endpoint (infrastructure, no auth) ---
    // The serialized metrics text is cached and regenerated by a dedicated
    // background thread. The /metrics endpoint just serves the cached string,
    // costing near-zero CPU on the poll thread.
    auto server_start_time = std::chrono::steady_clock::now();
    struct MetricsTextCache {
        std::mutex mutex;
        std::condition_variable cv;
        std::shared_ptr<const std::string> text = std::make_shared<const std::string>();
        bool requested = false;

        void store(std::shared_ptr<const std::string> t) {
            std::lock_guard lock(mutex);
            text = std::move(t);
        }
        std::shared_ptr<const std::string> load() {
            std::lock_guard lock(mutex);
            return text;
        }
        /// Signal the metrics thread to regenerate.
        void request_refresh() {
            {
                std::lock_guard lock(mutex);
                requested = true;
            }
            cv.notify_one();
        }
        /// Wait for a refresh request or timeout. Returns true if requested.
        bool wait_for_request(std::stop_token& st, std::chrono::milliseconds timeout) {
            std::unique_lock lock(mutex);
            cv.wait_for(lock, timeout, [&] { return requested || st.stop_requested(); });
            bool was_requested = requested;
            requested = false;
            return was_requested;
        }
    };
    auto metrics_cache = std::make_shared<MetricsTextCache>();
    std::jthread metrics_thread_handle; // kept alive for server lifetime
    std::atomic<WebSocketServer*> ws_ptr{nullptr}; // set after WS construction, read by metrics thread

    // WS poll thread utilization tracking (shared between WS thread and metrics thread)
    struct WsPollStats {
        std::atomic<uint64_t> idle_ns{0};
        std::atomic<uint64_t> busy_ns{0};
        std::atomic<uint64_t> frames_dispatched{0};
    } ws_poll_stats;

    if (cfg.metrics_enabled()) {
        http.GetInline(cfg.metrics_path(), [&metrics_cache](const http::Request&, http::Response& res) {
            metrics_cache->request_refresh();
            auto text = metrics_cache->load();
            res.set_content(*text, "text/plain; version=0.0.4; charset=utf-8");
        });
    }

    // --- REST API (AONX endpoints registered via EndpointFactory) ---
    nx_register_endpoints();
    NXEndpoint::set_server(&nx_backend);

    RestRouter rest_router;
    rest_router.set_cors_origins(cfg.cors_origins());
    rest_router.set_auth_func(
        [&room](const std::string& token) -> ServerSession* { return room.find_session_by_token(token); });
    EndpointFactory::instance().populate(rest_router);
    rest_router.bind(http);

    // --- Metrics snapshot collectors (registered after room + rest_router exist) ---
    if (cfg.metrics_enabled()) {
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
        auto& poll_util =
            reg.gauge("kagami_http_poll_utilization", "Poll thread utilization (0-1)");
        auto& poll_events = reg.gauge("kagami_http_poll_events_total", "Total events processed by poll thread");
        auto& poll_section_ns = reg.gauge("kagami_http_poll_section_nanoseconds_total",
                                          "Cumulative poll thread time per section in nanoseconds", {"section"});
        auto& worker_section_ns = reg.gauge("kagami_http_worker_section_nanoseconds_total",
                                            "Cumulative worker time per worker per section", {"worker", "section"});
        auto& io_uring_stats = reg.gauge("kagami_io_uring_ops_total",
                                         "io_uring operation counters", {"server", "op"});
        auto& ws_poll_util = reg.gauge("kagami_ws_poll_utilization",
                                       "WebSocket poll thread utilization (0-1)");
        auto& ws_dispatch_rate = reg.gauge("kagami_ws_dispatch_rate",
                                           "WebSocket frames dispatched per second");
        auto& lock_util = reg.gauge("kagami_dispatch_lock",
                                    "Dispatch mutex stats", {"type"});

        auto cors = cfg.cors_origins();
        server_info
            .labels({ao_sdl_version(), cfg.server_name(), std::to_string(cfg.max_players()),
                     std::to_string(cfg.session_ttl_seconds()), std::to_string(cfg.http_port()),
                     std::to_string(cfg.ws_port()), cfg.bind_address(), cors.empty() ? "" : cors[0]})
            .set(1);
        max_players.get().set(cfg.max_players());

        // Dedicated metrics thread — rebuilds gauges and serializes to text
        // every 1 second. The /metrics endpoint serves the cached string.
        // This keeps both the poll thread and worker pool free from metrics
        // overhead regardless of session count.
        std::jthread metrics_thread(
            [&uptime, &rss, &sessions_g, &sessions_joined, &sessions_mods, &area_players, &area_info, &chars_taken,
             &event_publishes, &session_bytes_sent, &session_bytes_recv, &session_packets_sent, &session_packets_recv,
             &session_idle, &http_open_conns, &http_work_queue, &http_result_queue, &http_active_workers,
             &http_worker_count, &http_worker_util, &http_worker_util_per, &cow_copy_bytes, &poll_util,
             &poll_events, &poll_section_ns, &worker_section_ns, &io_uring_stats,
             &ws_ptr, &ws_poll_stats, &ws_poll_util, &ws_dispatch_rate,
             &lock_util, &rest_router, &http_server = http, &room,
             &server_start_time, &reg, &metrics_cache](std::stop_token st) {
                // Previous cumulative values for delta computation
                uint64_t prev_worker_busy = 0, prev_worker_idle = 0;
                uint64_t prev_poll_busy = 0, prev_poll_idle = 0;
                size_t worker_count = 0;
                std::vector<uint64_t> prev_pw_busy;
                std::vector<uint64_t> prev_pw_idle;

                while (!st.stop_requested()) {
                    auto now = std::chrono::steady_clock::now();
                    uptime.get().set(std::chrono::duration<double>(now - server_start_time).count());
                    rss.get().set(static_cast<double>(metrics::process_rss_bytes()));

                    // HTTP worker pool metrics (lock-free — read atomics directly)
                    http_open_conns.get().set(static_cast<double>(http_server.open_connections()));
                    http_work_queue.get().set(static_cast<double>(http_server.work_queue_depth()));
                    http_result_queue.get().set(static_cast<double>(http_server.result_queue_depth()));
                    http_active_workers.get().set(static_cast<double>(http_server.active_workers()));
                    http_worker_count.get().set(static_cast<double>(http_server.worker_count()));

                    // Worker utilization — compute delta since last tick
                    {
                        auto cur_busy = http_server.worker_busy_ns();
                        auto cur_idle = http_server.worker_idle_ns();
                        auto d_busy = cur_busy - prev_worker_busy;
                        auto d_idle = cur_idle - prev_worker_idle;
                        auto d_total = d_busy + d_idle;
                        http_worker_util.get().set(d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0);
                        prev_worker_busy = cur_busy;
                        prev_worker_idle = cur_idle;
                    }

                    // Per-worker utilization (lazy init — workers start after metrics thread)
                    if (worker_count == 0) {
                        worker_count = http_server.worker_count();
                        prev_pw_busy.resize(worker_count, 0);
                        prev_pw_idle.resize(worker_count, 0);
                    }
                    for (size_t w = 0; w < worker_count; ++w) {
                        // idle is section index 0, busy = sum of dequeue(1) + handler(2) + result(3)
                        auto cur_idle = http_server.worker_section_ns(w, 0);
                        uint64_t cur_busy = 0;
                        for (size_t s = 1; s < http_server.worker_section_count(); ++s)
                            cur_busy += http_server.worker_section_ns(w, s);
                        auto d_busy = cur_busy - prev_pw_busy[w];
                        auto d_idle = cur_idle - prev_pw_idle[w];
                        auto d_total = d_busy + d_idle;
                        http_worker_util_per.labels({std::to_string(w)})
                            .set(d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0);
                        prev_pw_busy[w] = cur_busy;
                        prev_pw_idle[w] = cur_idle;
                    }

                    // Aggregate session stats (lock-free atomic reads)
                    sessions_g.labels({"ao2"}).set(room.stats.sessions_ao.load(std::memory_order_relaxed));
                    sessions_g.labels({"aonx"}).set(room.stats.sessions_nx.load(std::memory_order_relaxed));
                    sessions_joined.get().set(room.stats.joined.load(std::memory_order_relaxed));
                    sessions_mods.get().set(room.stats.moderators.load(std::memory_order_relaxed));
                    chars_taken.get().set(room.stats.chars_taken.load(std::memory_order_relaxed));
                    cow_copy_bytes.get().set(static_cast<double>(room.cow_copy_bytes()));

                    // Poll thread utilization — delta since last tick
                    {
                        auto cur_busy = http_server.poll_busy_ns();
                        auto cur_idle = http_server.poll_idle_ns();
                        auto d_busy = cur_busy - prev_poll_busy;
                        auto d_idle = cur_idle - prev_poll_idle;
                        auto d_total = d_busy + d_idle;
                        poll_util.get().set(d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0);
                        prev_poll_busy = cur_busy;
                        prev_poll_idle = cur_idle;
                    }
                    poll_events.get().set(static_cast<double>(http_server.poll_events_total()));
                    for (size_t i = 0; i < http_server.poll_section_count(); ++i)
                        poll_section_ns.labels({http_server.poll_section_name(i)})
                            .set(static_cast<double>(http_server.poll_section_ns(i)));

                    // io_uring stats for both HTTP and WS pollers
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
                    emit_io_stats("http", http_server.io_stats());
                    if (auto* wsp = ws_ptr.load(std::memory_order_acquire))
                        emit_io_stats("ws", wsp->io_stats());

                    // WS poll thread utilization (EMA-smoothed)
                    {
                        static uint64_t prev_ws_busy = 0, prev_ws_idle = 0;
                        static uint64_t prev_ws_dispatched = 0;
                        static auto prev_ws_time = std::chrono::steady_clock::now();
                        static double ema_util = 0.0;
                        constexpr double ALPHA = 0.1; // smoothing factor (lower = smoother)

                        auto cur_busy = ws_poll_stats.busy_ns.load(std::memory_order_relaxed);
                        auto cur_idle = ws_poll_stats.idle_ns.load(std::memory_order_relaxed);
                        auto cur_disp = ws_poll_stats.frames_dispatched.load(std::memory_order_relaxed);
                        auto cur_time = std::chrono::steady_clock::now();

                        auto d_busy = cur_busy - prev_ws_busy;
                        auto d_idle = cur_idle - prev_ws_idle;
                        auto d_total = d_busy + d_idle;
                        double instant_util = d_total > 0 ? static_cast<double>(d_busy) / d_total : 0.0;
                        ema_util = ALPHA * instant_util + (1.0 - ALPHA) * ema_util;
                        ws_poll_util.get().set(ema_util);

                        double dt = std::chrono::duration<double>(cur_time - prev_ws_time).count();
                        ws_dispatch_rate.get().set(dt > 0 ? (cur_disp - prev_ws_dispatched) / dt : 0.0);

                        prev_ws_busy = cur_busy;
                        prev_ws_idle = cur_idle;
                        prev_ws_dispatched = cur_disp;
                        prev_ws_time = cur_time;
                    }

                    // Dispatch lock stats (delta-based, computed server-side)
                    {
                        static uint64_t prev_excl_acq = 0, prev_excl_wait = 0, prev_excl_hold = 0;
                        static uint64_t prev_shared_acq = 0, prev_shared_wait = 0;

                        auto& ls = rest_router.lock_stats;
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

                        // Acquisitions per second
                        lock_util.labels({"exclusive_acquisitions_per_sec"}).set(d_excl_acq / 2.0);
                        lock_util.labels({"shared_acquisitions_per_sec"}).set(d_shared_acq / 2.0);
                        // Average wait per acquisition (nanoseconds)
                        lock_util.labels({"exclusive_avg_wait_ns"}).set(
                            d_excl_acq > 0 ? static_cast<double>(d_excl_wait) / d_excl_acq : 0.0);
                        lock_util.labels({"shared_avg_wait_ns"}).set(
                            d_shared_acq > 0 ? static_cast<double>(d_shared_wait) / d_shared_acq : 0.0);
                        // Hold time per second (ns of exclusive hold per second of wall time)
                        lock_util.labels({"exclusive_hold_ns_per_sec"}).set(d_excl_hold / 2.0);

                        prev_excl_acq = cur_excl_acq;
                        prev_excl_wait = cur_excl_wait;
                        prev_excl_hold = cur_excl_hold;
                        prev_shared_acq = cur_shared_acq;
                        prev_shared_wait = cur_shared_wait;
                    }

                    // Per-worker section breakdown (cumulative, for drill-down)
                    for (size_t w = 0; w < worker_count; ++w)
                        for (size_t s = 0; s < http_server.worker_section_count(); ++s)
                            worker_section_ns.labels({std::to_string(w), http_server.worker_section_name(s)})
                                .set(static_cast<double>(http_server.worker_section_ns(w, s)));

                    // Per-session + area detail (lock-free via COW snapshot)
                    auto snap = room.sessions_snapshot();

                    area_players.clear();
                    area_info.clear();
                    for (auto& [id, state] : room.area_states()) {
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
                        std::string char_name = (s->character_id >= 0 && s->character_id < (int)room.characters.size())
                                                    ? room.characters[s->character_id]
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

                    // Serialize and cache (no collectors needed — we populated gauges above)
                    auto text = std::make_shared<const std::string>(reg.collect());
                    metrics_cache->store(std::move(text));

                    // Wait for the next scrape request, or regenerate every 2s as a
                    // fallback (for dashboards that auto-refresh without scraping).
                    metrics_cache->wait_for_request(st, std::chrono::seconds(2));
                }
            });
        metrics_thread_handle = std::move(metrics_thread);
    }

    // Preflight handler for the SSE endpoint (registered outside RestRouter).
    http.Options("/aonx/v1/events", [](const http::Request&, http::Response& res) { res.status = 204; });

    // --- SSE endpoint (AONX Phase 5) ---
    http.SSE("/aonx/v1/events",
             [&rest_router, &room](const http::Request& req, http::Response& res) -> http::Server::SSEAcceptResult {
                 // Extract bearer token from Authorization header
                 auto auth = req.get_header_value("Authorization");
                 if (auth.size() <= 7 || auth.substr(0, 7) != "Bearer ") {
                     res.status = 401;
                     res.set_content(R"({"error":"Missing or invalid Authorization header"})", "application/json");
                     return {false, {}};
                 }
                 auto token = auth.substr(7);

                 // Validate session under dispatch lock
                 bool accepted = false;
                 rest_router.with_lock([&] {
                     auto* session = room.find_session_by_token(token);
                     if (session) {
                         session->touch();
                         accepted = true;
                     }
                 });
                 if (!accepted) {
                     res.status = 401;
                     res.set_content(R"({"error":"Invalid or expired session"})", "application/json");
                     return {false, {}};
                 }

                 return {true, token};
             });

    // Wire keepalive session touch — refreshes TTL for SSE-connected sessions
    http.set_sse_session_touch([&rest_router, &room](const std::string& token) {
        rest_router.with_lock([&] {
            auto* session = room.find_session_by_token(token);
            if (session)
                session->touch();
        });
    });

    if (!http.bind_to_port(cfg.bind_address(), cfg.http_port())) {
        Log::log_print(ERR, "Failed to bind HTTP on %s:%d", cfg.bind_address().c_str(), cfg.http_port());
        return 1;
    }
    Log::log_print(INFO, "HTTP listening on %s:%d", cfg.bind_address().c_str(), cfg.http_port());

    std::jthread http_thread([&](std::stop_token) { http.listen_after_bind(); });

    // --- WebSocket server + protocol routing ---
    auto listener = std::make_unique<PlatformServerSocket>(cfg.bind_address());
    WebSocketServer ws(std::move(listener));
    ws_ptr.store(&ws, std::memory_order_release);
    // Wire AO2 send function to WebSocket transport.
    // AONX clients use REST+SSE — no WebSocket involvement.
    auto ws_send = [&ws](uint64_t id, const std::string& data) {
        ws.send(id, {reinterpret_cast<const uint8_t*>(data.data()), data.size()});
    };
    ao_backend.set_send_func(ws_send);

    // WS connection gauge (registered here because ws is created after the main metrics block)
    if (cfg.metrics_enabled()) {
        auto& reg = metrics::MetricsRegistry::instance();
        auto& ws_conns = reg.gauge("kagami_ws_connections", "Active WebSocket connections");
        reg.add_collector([&ws_conns, &ws] { ws_conns.get().set(static_cast<double>(ws.client_count())); });
    }

    // All GameRoom mutations (WS and REST) share the dispatch mutex to
    // prevent concurrent access. This couples WS latency to REST handler
    // latency — acceptable at current scale, but worth revisiting if REST
    // handlers ever do slow work (e.g., asset I/O).
    ws.on_client_connected([&rest_router, &ao_backend](WebSocketServer::ClientId id) {
        rest_router.with_lock([&] { ao_backend.on_client_connected(id); });
    });
    ws.on_client_disconnected([&rest_router, &ao_backend](WebSocketServer::ClientId id) {
        rest_router.with_lock([&] { ao_backend.on_client_disconnected(id); });
    });

    ws.start(static_cast<uint16_t>(cfg.ws_port()));
    Log::log_print(INFO, "WebSocket listening on %s:%d", cfg.bind_address().c_str(), cfg.ws_port());

    // --- WS poll + session expiry thread ---
    std::jthread ws_thread([&](std::stop_token st) {
        auto last_sweep = std::chrono::steady_clock::now();
        while (!st.stop_requested()) {
            // Idle: time spent in poll() waiting for socket activity
            auto idle_start = std::chrono::steady_clock::now();
            auto frames = ws.poll(10);
            auto idle_end = std::chrono::steady_clock::now();
            ws_poll_stats.idle_ns.fetch_add(
                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(idle_end - idle_start).count()),
                std::memory_order_relaxed);

            // Busy: time spent dispatching frames.
            // Broadcasts are deferred: game state mutation happens inside the
            // lock, but the actual socket sends happen AFTER the lock is
            // released. This prevents 64 blocking sends from holding the
            // exclusive dispatch mutex and starving HTTP health checks.
            auto busy_start = std::chrono::steady_clock::now();

            using SendItem = std::pair<uint64_t, std::string>;
            std::vector<SendItem> pending_sends;
            auto capture_send = [&pending_sends](uint64_t id, const std::string& data) {
                pending_sends.emplace_back(id, data);
            };

            for (auto& [client_id, frame] : frames) {
                std::string data(frame.data.begin(), frame.data.end());
                Log::log_print(VERBOSE, "WS frame from %s: %s", format_client_id(client_id).c_str(), data.c_str());

                // Swap to deferred send during lock, restore after
                ao_backend.set_send_func(capture_send);
                rest_router.with_lock([&] { ao_backend.on_client_message(client_id, data); });
                ao_backend.set_send_func(ws_send);

                ws_poll_stats.frames_dispatched.fetch_add(1, std::memory_order_relaxed);
            }

            // Flush deferred sends outside the lock
            for (auto& [id, payload] : pending_sends)
                ws.send(id, {reinterpret_cast<const uint8_t*>(payload.data()), payload.size()});

            // Periodic session expiry sweep (~every 30s).
            // Two-phase: scan under shared lock (cheap, doesn't block requests),
            // then destroy in batches under exclusive lock. Batching prevents
            // holding the lock for seconds when thousands of sessions expire.
            auto now = std::chrono::steady_clock::now();
            if (now - last_sweep > std::chrono::seconds(30)) {
                std::vector<uint64_t> to_expire;
                rest_router.with_shared_lock(
                    [&] { to_expire = room.find_expired_sessions(cfg.session_ttl_seconds()); });
                if (!to_expire.empty()) {
                    Log::log_print(INFO, "Expiring %zu sessions", to_expire.size());
                    constexpr size_t BATCH_SIZE = 64;
                    for (size_t i = 0; i < to_expire.size(); i += BATCH_SIZE) {
                        size_t end = std::min(i + BATCH_SIZE, to_expire.size());
                        rest_router.with_lock([&] {
                            for (size_t j = i; j < end; ++j)
                                room.destroy_session(to_expire[j]);
                            room.broadcast_chars_taken();
                        });
                        // Yield between batches so HTTP requests can be served
                    }
                    metrics::MetricsRegistry::instance()
                        .counter("kagami_sessions_expired_total", "Sessions expired by TTL")
                        .get()
                        .inc(to_expire.size());
                }
                last_sweep = now;
            }

            ws_poll_stats.busy_ns.fetch_add(
                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - busy_start).count()),
                std::memory_order_relaxed);
        }
    });

    // --- REPL ---
    ReplCommandRegistry repl;

    repl.add({"/stop", "Shut down the server", [&](auto&) { stop_src.request_stop(); }});

    repl.add({"/help", "List available commands", [&](auto&) {
                  for (auto& [name, cmd] : repl.commands()) {
                      ui.print("  " + name + " — " + cmd.description);
                  }
              }});

    repl.add({"/sessions", "List active sessions", [&](auto&) {
                  rest_router.with_lock([&] {
                      if (room.session_count() == 0) {
                          ui.print("  No active sessions.");
                          return;
                      }
                      room.for_each_session([&](const ServerSession& s) {
                          std::string info = "  " + format_client_id(s.client_id);
                          if (!s.display_name.empty())
                              info += " \"" + s.display_name + "\"";
                          if (s.character_id >= 0)
                              info += " char=" + std::to_string(s.character_id);
                          if (!s.area.empty())
                              info += " area=" + s.area;
                          if (!s.client_software.empty())
                              info += " (" + s.client_software + ")";
                          ui.print(info);
                      });
                      ui.print("  Total: " + std::to_string(room.session_count()));
                  });
              }});

    repl.add({"/status", "Show server status", [&](auto&) {
                  ui.print("Server:     " + cfg.server_name());
                  ui.print("HTTP:       " + cfg.bind_address() + ":" + std::to_string(cfg.http_port()));
                  ui.print("WS:         " + cfg.bind_address() + ":" + std::to_string(cfg.ws_port()));
                  ui.print("WS clients: " + std::to_string(ws.client_count()));
              }});

    if (interactive) {
        std::string line;
        while (!stop_src.stop_requested() && std::getline(std::cin, line)) {
            if (!line.empty() && !repl.dispatch(line))
                ui.print("Unknown command: " + line + " (try /help)");
            ui.show_prompt();
        }
        ui.cleanup();
    }
    else {
        while (!stop_src.stop_requested())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // --- Shutdown ---
    stop_src.request_stop();
    Log::log_print(INFO, "Shutting down...");

    // Remove sinks before stopping backends so the callbacks can't fire on
    // a stopped/destroyed object. CloudWatch's stop() does a final flush()
    // which sends buffered events via HTTP and reports errors to stderr
    // directly (not through Log), so this ordering is safe.
    Log::remove_sink("loki");
    Log::remove_sink("cloudwatch");
    Log::remove_sink("file");
    Log::set_sink(nullptr);
    if (loki_sink)
        loki_sink->stop();
    if (cw_sink)
        cw_sink->stop();

    ws.stop();
    http.stop();

    ServerSettings::save_to_disk(cfg_path);

    return 0;
}
