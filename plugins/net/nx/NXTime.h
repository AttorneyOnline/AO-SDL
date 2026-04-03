#pragma once

#include <chrono>
#include <format>
#include <string>

/// Format a system_clock time_point as ISO 8601 (UTC).
/// Uses C++20 chrono calendar types — no gmtime_r/gmtime_s portability issues.
inline std::string to_iso8601(std::chrono::system_clock::time_point tp) {
    auto dp = std::chrono::floor<std::chrono::days>(tp);
    std::chrono::year_month_day ymd{dp};
    std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::seconds>(tp - dp)};
    return std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z", static_cast<int>(ymd.year()),
                       static_cast<unsigned>(ymd.month()), static_cast<unsigned>(ymd.day()), hms.hours().count(),
                       hms.minutes().count(), hms.seconds().count());
}
