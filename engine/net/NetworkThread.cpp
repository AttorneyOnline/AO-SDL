#include "net/NetworkThread.h"

#include "net/WebSocket.h"
#include "event/EventManager.h"
#include "event/ServerConnectEvent.h"
#include "utils/Log.h"

#include <chrono>
#include <optional>
#include <thread>

NetworkThread::NetworkThread(ProtocolHandler& handler)
    : handler(handler), net_thread(&NetworkThread::net_loop, this) {
}

void NetworkThread::net_loop() {
    // Wait for the user to select a server before connecting.
    std::optional<WebSocket> sock;
    while (!sock.has_value()) {
        if (auto ev = EventManager::instance().get_channel<ServerConnectEvent>().get_event()) {
            sock.emplace(ev->get_host(), ev->get_port());
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // todo: sock.connect() is blocking — implement a timeout
    if (!sock->is_connected()) {
        sock->connect();
    }

    handler.on_connect();

    std::vector<WebSocket::WebSocketFrame> msgs;

    while (true) {
        // todo: cleanly handle and stitch continuation frames
        msgs = sock->read();

        for (const auto& msg : msgs) {
            std::string msgstr(msg.data.begin(), msg.data.end());
            Log::log_print(DEBUG, "SERVER: %s", msgstr.c_str());
            handler.on_message(msgstr);
        }
        msgs.clear();

        for (const auto& out : handler.flush_outgoing()) {
            Log::log_print(DEBUG, "CLIENT: %s", out.c_str());
            sock->write(std::vector<uint8_t>(out.begin(), out.end()));
        }

        // todo: make this timeout less stupid
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    handler.on_disconnect();
}
