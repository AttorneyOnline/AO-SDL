#include "platform/Time.h"

#include <ctime>

namespace platform {

std::chrono::local_seconds to_local(std::chrono::system_clock::time_point tp) {
    // Apple libc++ does not implement the C++20 chrono timezone library
    // (std::chrono::current_zone, std::chrono::zoned_time, etc.) as of
    // Xcode 16 / Apple Clang 17. Fall back to POSIX localtime_r.
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm local{};
    localtime_r(&tt, &local);

    // Reconstruct a local_seconds from the broken-down fields.
    using namespace std::chrono;
    auto ymd = year{local.tm_year + 1900} / (local.tm_mon + 1) / local.tm_mday;
    auto tod = hours{local.tm_hour} + minutes{local.tm_min} + seconds{local.tm_sec};
    return local_days{ymd} + tod;
}

} // namespace platform
