#pragma once

#include <string>
#include <vector>

class ProtocolHandler {
  public:
    virtual ~ProtocolHandler() = default;

    // Called once after the transport connection is established.
    virtual void on_connect() = 0;

    // Called for each complete message received from the server.
    virtual void on_message(const std::string& msg) = 0;

    // Called when the transport connection is lost.
    virtual void on_disconnect() = 0;

    // Returns any outgoing messages queued since the last call, then clears the queue.
    // NetworkThread drains this every tick and writes each message to the socket.
    virtual std::vector<std::string> flush_outgoing() = 0;
};
