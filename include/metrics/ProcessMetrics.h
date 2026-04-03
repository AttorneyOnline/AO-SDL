/**
 * @file ProcessMetrics.h
 * @brief Platform-specific process metrics (RSS memory usage).
 */
#pragma once

#include <cstdint>

#ifdef __APPLE__
#include <mach/mach.h>
#elif defined(__linux__)
#include <cstdio>
#include <cstring>
#endif

namespace metrics {

/// Returns the resident set size (physical memory usage) in bytes.
/// Returns 0 if the platform is unsupported or the query fails.
inline uint64_t process_rss_bytes() {
#ifdef __APPLE__
    mach_task_basic_info_data_t info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return info.resident_size;
    return 0;
#elif defined(__linux__)
    FILE* f = std::fopen("/proc/self/status", "r");
    if (!f)
        return 0;
    char line[256];
    uint64_t rss_kb = 0;
    while (std::fgets(line, sizeof(line), f)) {
        if (std::strncmp(line, "VmRSS:", 6) == 0) {
            std::sscanf(line + 6, " %lu", &rss_kb);
            break;
        }
    }
    std::fclose(f);
    return rss_kb * 1024;
#else
    return 0;
#endif
}

} // namespace metrics
