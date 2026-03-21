#pragma once

#include <string>
#include <vector>

namespace platform {

/// Return paths to system fonts suitable for Unicode fallback, in priority order.
/// Implemented per-platform in platform/{macos,windows,linux}.
std::vector<std::string> fallback_font_paths();

} // namespace platform
