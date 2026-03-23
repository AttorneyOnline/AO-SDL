#pragma once

#include <chrono>

namespace platform {

/// Convert a system_clock time_point to a local time_point.
///
/// On Windows/Linux this uses C++20 std::chrono::current_zone()->to_local(),
/// which MSVC and libstdc++ support. Apple's libc++ does not yet implement the
/// C++20 timezone library (as of Xcode 16 / Apple Clang 17), so the macOS
/// build falls back to localtime_r via POSIX.
std::chrono::local_seconds to_local(std::chrono::system_clock::time_point tp);

} // namespace platform
