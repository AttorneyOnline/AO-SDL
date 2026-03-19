#pragma once

#include "net/ProtocolHandler.h"

#include <memory>

// =============================================================================
// AO2 protocol plugin — public entry point
//
// Applications call create_ao_protocol() to get a ProtocolHandler they can
// pass to NetworkThread. No other AO-internal headers need to be included.
//
// Usage:
//   auto protocol = ao::create_protocol();
//   NetworkThread net_thread(*protocol);
// =============================================================================

namespace ao {

std::unique_ptr<ProtocolHandler> create_protocol();

} // namespace ao
