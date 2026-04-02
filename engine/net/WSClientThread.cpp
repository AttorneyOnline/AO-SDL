#include "net/WSClientThread.h"

#include "event/DisconnectEvent.h"
#include "event/DisconnectRequestEvent.h"
#include "event/EventManager.h"
#include "event/ServerConnectEvent.h"
#include "event/SessionEndEvent.h"
#include "event/SessionStartEvent.h"
#include "net/PlatformTcpSocket.h"
#include "net/WebSocket.h"
#include "platform/Poll.h"
#include "utils/Log.h"

#include <chrono>
#include <optional>
#include <thread>

// ---------------------------------------------------------------------------
// Scheme parsing for ws:// / wss:// URLs
// ---------------------------------------------------------------------------

ParsedWsUrl parse_ws_url(const std::string& host_in, uint16_t port_in) {
    ParsedWsUrl result;
    std::string rest = host_in;

    // Check for scheme prefix
    if (rest.starts_with("wss://")) {
        result.ssl = true;
        rest = rest.substr(6);
        result.port = (port_in != 0) ? port_in : 443;
    }
    else if (rest.starts_with("ws://")) {
        result.ssl = false;
        rest = rest.substr(5);
        result.port = (port_in != 0) ? port_in : 80;
    }
    else {
        result.ssl = false;
        result.port = port_in;
    }

    // Strip trailing path if present (e.g. "host.com/ws")
    auto slash = rest.find('/');
    if (slash != std::string::npos) {
        rest = rest.substr(0, slash);
    }

    // Extract host:port if port is embedded
    auto colon = rest.rfind(':');
    if (colon != std::string::npos && colon > 0) {
        // Could be IPv6 — only parse port if there's no '[' before the colon
        if (rest[0] != '[' || rest.find(']') < colon) {
            result.host = rest.substr(0, colon);
            try {
                result.port = static_cast<uint16_t>(std::stoi(rest.substr(colon + 1)));
            }
            catch (const std::exception&) {
                Log::log_print(ERR, "parse_ws_url: invalid port in '%s', using default %d", host_in.c_str(),
                               result.port);
            }
        }
        else {
            result.host = rest;
        }
    }
    else {
        result.host = rest;
    }

    return result;
}

// ---------------------------------------------------------------------------
// WSClientThread
// ---------------------------------------------------------------------------

WSClientThread::WSClientThread(ProtocolHandler& handler)
    : handler(handler), thread_([this](std::stop_token st) { ws_loop(st); }) {
}

void WSClientThread::stop() {
    thread_.request_stop();
    if (thread_.joinable())
        thread_.join();
}

void WSClientThread::ws_loop(std::stop_token st) {
    while (!st.stop_requested()) {
        // Drain stale disconnect requests before waiting for a new connection.
        while (EventManager::instance().get_channel<DisconnectRequestEvent>().get_event()) {
        }

        // Wait for the user to select a server, using a Poller notifier
        // to wake instantly when a ServerConnectEvent is published.
        std::optional<WebSocket> sock;
        ParsedWsUrl url;

        platform::Poller connect_poller;
        connect_poller.create_notifier();
        auto& connect_channel = EventManager::instance().get_channel<ServerConnectEvent>();
        connect_channel.set_on_publish([&connect_poller] { connect_poller.notify(); });

        while (!st.stop_requested() && !sock.has_value()) {
            // Check for events before blocking — one may already be queued
            if (auto ev = connect_channel.get_event()) {
                url = parse_ws_url(ev->get_host(), ev->get_port());

                auto tcp = std::make_unique<PlatformTcpSocket>(url.host, url.port);
                if (url.ssl) {
                    tcp->enable_ssl(url.host);
                }
                sock.emplace(url.host, url.port, std::move(tcp));
            }
            else {
                // Block until notified (with timeout so stop_requested is checked)
                platform::Poller::Event events[1];
                connect_poller.poll(events, 1, 200);
                connect_poller.drain_notifier();
            }
        }

        connect_channel.set_on_publish(nullptr);

        if (st.stop_requested())
            return;

        try {
            if (!sock->is_connected()) {
                sock->connect();
                Log::log_print(INFO, "Connected to %s:%d%s", url.host.c_str(), url.port, url.ssl ? " (TLS)" : "");
            }
        }
        catch (const std::exception& e) {
            Log::log_print(ERR, "Connection failed: %s", e.what());
            EventManager::instance().get_channel<DisconnectEvent>().publish(DisconnectEvent(e.what()));
            continue;
        }

        // All ProtocolHandler methods are called exclusively from this thread.
        // Implementations must be aware that these run on the network thread,
        // not the main/UI thread.
        handler.on_connect();
        EventManager::instance().get_channel<SessionStartEvent>().publish(SessionStartEvent());

        // Drain disconnect requests that arrived during handshake.
        while (EventManager::instance().get_channel<DisconnectRequestEvent>().get_event()) {
        }

        // Set up Poller for the message loop: watch the socket fd for data,
        // use a notifier to wake when disconnect is requested or outgoing
        // messages need flushing.
        platform::Poller poller;
        int notifier_fd = poller.create_notifier();
        int sock_fd = sock->socket_fd();
        if (sock_fd >= 0)
            poller.add(sock_fd, platform::Poller::Readable);

        // Wire disconnect-request and flush notifications to the poller
        auto& disconnect_channel = EventManager::instance().get_channel<DisconnectRequestEvent>();
        disconnect_channel.set_on_publish([&poller] { poller.notify(); });

        std::vector<WebSocket::WebSocketFrame> msgs;

        while (!st.stop_requested()) {
            // Block until socket is readable, notifier fires, or timeout
            // The 200ms timeout is a safety net for stop_requested checks;
            // normal wakeups are instant via the Poller.
            platform::Poller::Event events[4];
            int n = poller.poll(events, 4, 200);

            // Check for disconnect request
            if (disconnect_channel.get_event()) {
                Log::log_print(INFO, "Disconnect requested by user");
                break;
            }

            // Drain notifier if it fired
            for (int i = 0; i < n; ++i) {
                if (events[i].fd == notifier_fd)
                    poller.drain_notifier();
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
        }

        disconnect_channel.set_on_publish(nullptr);

        handler.on_disconnect();
        EventManager::instance().get_channel<SessionEndEvent>().publish(SessionEndEvent());
    }
}
