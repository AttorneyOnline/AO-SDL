#pragma once

#include <string>

namespace platform {

/// Return a stable, hashed hardware fingerprint for this machine.
/// The raw hardware identifier is SHA-256 hashed before returning,
/// so no personally identifiable information (serial numbers, SIDs)
/// is transmitted over the network.
///
/// Implemented per-platform in platform/{macos,windows,linux}.
std::string hardware_id();

} // namespace platform
