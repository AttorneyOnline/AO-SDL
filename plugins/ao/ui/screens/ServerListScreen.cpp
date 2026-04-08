#include "ao/ui/screens/ServerListScreen.h"

#include "ao/ui/screens/CharSelectScreen.h"
#include "event/EventManager.h"
#include "event/ServerConnectEvent.h"
#include "event/ServerListEvent.h"

void ServerListScreen::enter(ScreenController& ctrl) {
    controller = &ctrl;
}

void ServerListScreen::exit() {
    controller = nullptr;
}

void ServerListScreen::handle_events() {
    auto& list_channel = EventManager::instance().get_channel<ServerListEvent>();
    while (auto optev = list_channel.get_event()) {
        servers = optev->get_server_list().get_servers();
    }

    if (pending_connect) {
        pending_connect = false;
        controller->push_screen(std::make_unique<CharSelectScreen>());
    }
}

void ServerListScreen::select_server(int index) {
    if (index < 0 || index >= (int)servers.size())
        return;

    const auto& s = servers[index];

    // Prefer WSS (encrypted) over plain WS when available.
    if (s.wss_port.has_value()) {
        selected = index;
        pending_connect = true;
        EventManager::instance().get_channel<ServerConnectEvent>().publish(
            ServerConnectEvent("wss://" + s.hostname, *s.wss_port));
        return;
    }

    if (s.ws_port.has_value()) {
        selected = index;
        pending_connect = true;
        EventManager::instance().get_channel<ServerConnectEvent>().publish(ServerConnectEvent(s.hostname, *s.ws_port));
        return;
    }
}

void ServerListScreen::direct_connect(const std::string& host, uint16_t port) {
    pending_connect = true;
    EventManager::instance().get_channel<ServerConnectEvent>().publish(ServerConnectEvent(host, port));
}
