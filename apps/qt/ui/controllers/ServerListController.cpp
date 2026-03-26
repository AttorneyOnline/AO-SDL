#include "ServerListController.h"

#include "ao/ui/screens/ServerListScreen.h"
#include "ui/Screen.h"

ServerListController::ServerListController(QObject* parent)
    : IQtScreenController(parent) {}

void ServerListController::sync(Screen& screen) {
    m_screen = static_cast<ServerListScreen*>(&screen);
    m_model.reset(m_screen->get_servers());
}

void ServerListController::connectToServer(int index) {
    if (m_screen)
        m_screen->select_server(index);
}

void ServerListController::directConnect(const QString& host, quint16 port) {
    if (m_screen)
        m_screen->direct_connect(host.toStdString(), port);
}
