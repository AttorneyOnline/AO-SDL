#pragma once

#include "game/IScenePresenter.h"
#include "net/ProtocolHandler.h"

#include <memory>

// =============================================================================
// AO2 protocol plugin — public entry point
//
// Applications call the factory functions below to get protocol and scene
// presenter instances. No AO-internal headers need to be included.
//
// Usage:
//   auto protocol  = ao::create_protocol();
//   auto presenter = ao::create_presenter();
//   NetworkThread net_thread(*protocol);
//   GameThread game_logic(buffer, *presenter);
// =============================================================================

namespace ao {

/// Create the AO2 protocol handler (parses AO packets, publishes events).
std::unique_ptr<ProtocolHandler> create_protocol();

/// Create the AO2 courtroom scene presenter (consumes events, renders scene).
std::unique_ptr<IScenePresenter> create_presenter();

} // namespace ao
