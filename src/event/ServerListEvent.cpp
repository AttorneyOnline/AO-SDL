#include "ServerListEvent.h"

#include <format>

ServerListEvent::ServerListEvent(ServerList server_list, EventTarget target) : server_list(server_list), Event(target) {
}

std::string ServerListEvent::to_string() const {
    std::string serverlist_str;

    for (auto server : server_list.get_servers()) {
        serverlist_str = std::format("{}\n{}:{}", serverlist_str, server.hostname, server.ws_port);
    }

    return serverlist_str;
}

ServerList ServerListEvent::get_server_list() {
    return server_list;
}