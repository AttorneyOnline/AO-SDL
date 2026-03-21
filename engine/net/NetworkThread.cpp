#include "net/NetworkThread.h"

#include "event/DisconnectEvent.h"
#include "event/EventManager.h"
#include "event/ServerConnectEvent.h"
#include "net/WebSocket.h"
#include "utils/Log.h"

#include <chrono>
#include <optional>
#include <thread>

NetworkThread::NetworkThread(ProtocolHandler& handler)
    : running(true), handler(handler), net_thread(&NetworkThread::net_loop, this) {
}

void NetworkThread::stop() {
    running = false;
    if (net_thread.joinable()) {
        net_thread.join();
    }
}

void NetworkThread::net_loop() {
    while (running) {
        // Wait for the user to select a server before connecting.
        std::optional<WebSocket> sock;
        while (running && !sock.has_value()) {
            if (auto ev = EventManager::instance().get_channel<ServerConnectEvent>().get_event()) {
                sock.emplace(ev->get_host(), ev->get_port());
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        if (!running)
            return;

        try {
            if (!sock->is_connected()) {
                sock->connect();
            }
        }
        catch (const std::exception& e) {
            Log::log_print(ERR, "Connection failed: %s", e.what());
            EventManager::instance().get_channel<DisconnectEvent>().publish(DisconnectEvent(e.what()));
            continue;
        }

        handler.on_connect();

        std::vector<WebSocket::WebSocketFrame> msgs;

        while (running) {
            try {
                msgs = sock->read();

                for (const auto& msg : msgs) {
                    std::string msgstr(msg.data.begin(), msg.data.end());
                    Log::log_print(VERBOSE, "SERVER: %s", msgstr.c_str());
                    handler.on_message(msgstr);
                }
                msgs.clear();

                for (const auto& out : handler.flush_outgoing()) {
                    Log::log_print(VERBOSE, "CLIENT: %s", out.c_str());
                    sock->write(std::vector<uint8_t>(out.begin(), out.end()));
                }
            }
            catch (const std::exception& e) {
                Log::log_print(ERR, "Connection lost: %s", e.what());
                EventManager::instance().get_channel<DisconnectEvent>().publish(DisconnectEvent(e.what()));
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        handler.on_disconnect();
        // Socket goes out of scope here, loop back to wait for next ServerConnectEvent
    }
}
