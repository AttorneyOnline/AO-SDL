#pragma once

#include "IQtScreenController.h"
#include "game/ServerList.h"
#include "ui/models/ServerListModel.h"

#include <QObject>
#include <QString>

#include <vector>

class UIManager;

/**
 * @brief Qt controller for the server browser screen.
 *
 * drain() consumes ServerListEvent directly from EventManager and updates
 * ServerListModel — no Screen object is involved.
 *
 * connectToServer() and directConnect() publish ServerConnectEvent and push
 * CharSelectScreen onto UIManager's stack, making UIManager the single
 * authority over navigation.
 */
class ServerListController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(ServerListModel* model READ model CONSTANT)

  public:
    explicit ServerListController(UIManager& uiMgr, QObject* parent = nullptr);

    /// IQtScreenController
    void drain() override;

    ServerListModel* model() { return &m_model; }

    /// Connect to the server at the given index in the model.
    Q_INVOKABLE void connectToServer(int index);

    /// Connect directly to host:port without going through the server list.
    Q_INVOKABLE void directConnect(const QString& host, quint16 port);

  private:
    void doConnect(const std::string& host, uint16_t port);

    UIManager&               m_uiMgr;
    ServerListModel          m_model;
    std::vector<ServerEntry> m_entries; ///< Mirrored from ServerListEvent for connectToServer().
};
