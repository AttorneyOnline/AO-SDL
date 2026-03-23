#include "platform/Time.h"

namespace platform {

std::chrono::local_seconds to_local(std::chrono::system_clock::time_point tp) {
    // libstdc++ (GCC) supports the C++20 chrono timezone library.
    return std::chrono::floor<std::chrono::seconds>(std::chrono::current_zone()->to_local(tp));
}

} // namespace platform
