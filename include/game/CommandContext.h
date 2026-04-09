/**
 * @file CommandContext.h
 * @brief Context passed to OOC command handlers.
 *
 * CommandContext carries everything a command needs to execute: the game room,
 * invoking session, parsed arguments, and protocol-specific callbacks for
 * sending messages and disconnecting clients. The callbacks allow commands
 * to be protocol-agnostic -- AO2 fills them with packet sends, NX fills
 * them with JSON responses.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class GameRoom;
struct ServerSession;

struct CommandContext {
    GameRoom& room;
    ServerSession& session;

    /// Parsed arguments. args[0] is the command name (e.g. "kick"),
    /// args[1..] are the arguments.
    std::vector<std::string> args;

    /// Send a system OOC message to the invoking client.
    std::function<void(const std::string& message)> send_system_message;

    /// Send a system OOC message to a specific client by ID.
    std::function<void(uint64_t client_id, const std::string& message)> send_system_message_to;

    /// Force-disconnect a client by ID.
    std::function<void(uint64_t client_id)> disconnect_client;

    /// Send a kick notification to a client, then disconnect.
    std::function<void(uint64_t client_id, const std::string& reason)> send_kick_message;

    /// Send a ban notification to a client, then disconnect.
    std::function<void(uint64_t client_id, const std::string& reason)> send_ban_message;

    /// Send area join info (background, HP, evidence) to a specific client.
    std::function<void(uint64_t client_id, const std::string& area)> send_area_join_info;

    /// Broadcast a background change to all clients in an area.
    std::function<void(const std::string& area, const std::string& bg)> broadcast_background;

    /// Broadcast an area update (ARUP) to all clients. type: 0=players, 1=status, 2=cm, 3=locked.
    std::function<void(int arup_type)> broadcast_arup;

    /// Client IDs to disconnect after the command finishes.
    /// Commands push IDs here; the protocol backend closes them after
    /// try_dispatch returns, outside any locks that would deadlock
    /// with the on_disconnected callback.
    std::vector<uint64_t> deferred_closes;
};
