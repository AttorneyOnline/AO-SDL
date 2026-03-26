#include "net/NetworkThread.h"

#include "event/DisconnectEvent.h"
#include "event/DisconnectRequestEvent.h"
#include "event/EventManager.h"
#include "event/ServerConnectEvent.h"
#include "event/SessionEndEvent.h"
#include "event/SessionStartEvent.h"
#include "net/WebSocket.h"
#include "utils/Log.h"

#include <chrono>
#include <optional>
#include <thread>

NetworkThread::NetworkThread(ProtocolHandler& handler)
    : handler(handler), net_thread([this](std::stop_token st) { net_loop(st); }) {
}

void NetworkThread::stop() {
    net_thread.request_stop();
    if (net_thread.joinable())
        net_thread.join();
}

void NetworkThread::net_loop(std::stop_token st) {
    while (!st.stop_requested()) {
        // Drain any stale disconnect requests before waiting for a new connection.
        while (EventManager::instance().get_channel<DisconnectRequestEvent>().get_event()) {
        }

        // Wait for the user to select a server before connecting.
        std::optional<WebSocket> sock;
        while (!st.stop_requested() && !sock.has_value()) {
            if (auto ev = EventManager::instance().get_channel<ServerConnectEvent>().get_event()) {
                sock.emplace(ev->get_host(), ev->get_port());
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        if (st.stop_requested())
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
        EventManager::instance().get_channel<SessionStartEvent>().publish(SessionStartEvent());

        std::vector<WebSocket::WebSocketFrame> msgs;

        while (!st.stop_requested()) {
            // Check if the UI requested a voluntary disconnect.
            if (EventManager::instance().get_channel<DisconnectRequestEvent>().get_event()) {
                Log::log_print(INFO, "Disconnect requested by user");
                break;
            }

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
        EventManager::instance().get_channel<SessionEndEvent>().publish(SessionEndEvent());
        // Socket goes out of scope here, loop back to wait for next ServerConnectEvent
    }
}
