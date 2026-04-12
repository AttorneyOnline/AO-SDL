#include "ServerListController.h"

#include "ao/ui/screens/CharSelectScreen.h"
#include "event/EventManager.h"
#include "event/ServerConnectEvent.h"
#include "event/ServerListEvent.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

#include <memory>

ServerListController::ServerListController(UIManager& uiMgr, QObject* parent)
    : IQtScreenController(parent), m_uiMgr(uiMgr) {
    m_filterProxy.setSourceModel(&m_model);
}

void ServerListController::drain() {
    auto& ch = EventManager::instance().get_channel<ServerListEvent>();
    while (auto ev = ch.get_event()) {
        m_entries = ev->get_server_list().get_servers();
        m_model.reset(m_entries);
        Log::info("[ServerListController] received server list ({} servers)", m_entries.size());
    }
}

void ServerListController::connectToServer(int proxyRow) {
    const QModelIndex sourceIdx = m_filterProxy.mapToSource(m_filterProxy.index(proxyRow, 0));
    const int index = sourceIdx.row();
    if (index < 0 || index >= static_cast<int>(m_entries.size()))
        return;

    const ServerEntry& entry = m_entries[index];

    // Prefer WebSocket; fall back to TCP if no WS port is advertised.
    uint16_t port = 0;
    if (entry.ws_port)
        port = *entry.ws_port;
    else if (entry.tcp_port)
        port = *entry.tcp_port;

    if (port == 0)
        return;

    doConnect(entry.hostname, port);
}

void ServerListController::directConnect(const QString& host, quint16 port) {
    doConnect(host.toStdString(), port);
}

void ServerListController::doConnect(const std::string& host, uint16_t port) {
    Log::info("[ServerListController] connecting to {}:{}", host, port);
    EventManager::instance().get_channel<ServerConnectEvent>().publish(ServerConnectEvent(host, port));
    m_uiMgr.push_screen(std::make_unique<CharSelectScreen>());
}
