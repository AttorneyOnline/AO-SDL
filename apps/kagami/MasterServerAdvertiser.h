#pragma once

#include "ServerSettings.h"
#include "game/GameRoom.h"

#include <stop_token>
#include <thread>

/// Periodically advertises this server to the AO master server list.
///
/// Sends an HTTP POST with JSON describing the server (name, description,
/// player count, ports) on a fixed interval. Mirrors the akashi
/// ServerPublisher behaviour but uses std::jthread + http::Client instead
/// of Qt networking.
class MasterServerAdvertiser {
  public:
    MasterServerAdvertiser(const ServerSettings& cfg, const GameRoom& room);
    ~MasterServerAdvertiser();

    /// Start the background advertising thread. No-op if advertising is
    /// disabled in config or already running.
    void start();

    /// Request stop and join the thread. Safe to call multiple times.
    void stop();

  private:
    void run(std::stop_token stoken);
    bool publish_once();

    const ServerSettings& cfg_;
    const GameRoom& room_;
    std::jthread thread_;
};
