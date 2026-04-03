#include "utils/Log.h"

#include "platform/Time.h"

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

// ---- Timestamp formatting via std::chrono ----------------------------------

std::string LogEvent::timestamp() const {
    auto local = platform::to_local(time);
    auto dp = std::chrono::floor<std::chrono::days>(local);
    auto tod = std::chrono::hh_mm_ss(std::chrono::floor<std::chrono::seconds>(local - dp));

    int h = tod.hours().count();
    int m = tod.minutes().count();
    int s = tod.seconds().count();
    const char* ampm = (h >= 12) ? "PM" : "AM";
    if (h == 0)
        h = 12;
    else if (h > 12)
        h -= 12;

    return std::format("{:02d}:{:02d}:{:02d} {}", h, m, s, ampm);
}

// ---- Sink management -------------------------------------------------------

static std::mutex sink_mutex;
static Log::Sink log_sink;
static LogLevel log_sink_level = VERBOSE;
static LogLevel stdout_level = VERBOSE;

struct NamedSink {
    std::string name;
    Log::Sink sink;
    LogLevel min_level;
};
static std::vector<NamedSink> named_sinks;

void Log::set_sink(Sink sink, LogLevel min_level) {
    std::lock_guard lock(sink_mutex);
    log_sink = std::move(sink);
    log_sink_level = min_level;
}

void Log::add_sink(const std::string& name, Sink sink, LogLevel min_level) {
    std::lock_guard lock(sink_mutex);
    // Replace if name already exists
    for (auto& ns : named_sinks) {
        if (ns.name == name) {
            ns.sink = std::move(sink);
            ns.min_level = min_level;
            return;
        }
    }
    named_sinks.push_back({name, std::move(sink), min_level});
}

void Log::remove_sink(const std::string& name) {
    std::lock_guard lock(sink_mutex);
    std::erase_if(named_sinks, [&](const NamedSink& ns) { return ns.name == name; });
}

void Log::set_stdout_level(LogLevel min_level) {
    std::lock_guard lock(sink_mutex);
    stdout_level = min_level;
}

void Log::clear_sinks() {
    std::lock_guard lock(sink_mutex);
    log_sink = nullptr;
    named_sinks.clear();
}

// ---- Core log implementation -----------------------------------------------

void Log::log_impl(LogLevel level, std::string message) {
    if (level >= LogLevel::COUNT || level <= LogLevel::INVALID)
        return;

    LogEvent event{level, std::chrono::system_clock::now(), std::move(message)};
    auto ts = event.timestamp();

    // Callback sinks (thread-safe)
    {
        std::lock_guard lock(sink_mutex);

        // stdout
        if (level >= stdout_level)
            std::printf("[%s][%s] %s\n", ts.c_str(), log_level_name(level), event.message.c_str());

        if (log_sink && level >= log_sink_level)
            log_sink(level, ts, event.message);
        for (auto& ns : named_sinks) {
            if (level >= ns.min_level)
                ns.sink(level, ts, event.message);
        }
    }

    if (level >= LogLevel::FATAL)
        std::exit(1);
}

// ---- Legacy printf API (compatibility) -------------------------------------

void Log::log_print(LogLevel log_level, const char* fmt, ...) {
    if (log_level >= LogLevel::COUNT || log_level <= LogLevel::INVALID)
        return;

    char messagebuf[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(messagebuf, sizeof(messagebuf), fmt, args);
    va_end(args);

    log_impl(log_level, messagebuf);
}
