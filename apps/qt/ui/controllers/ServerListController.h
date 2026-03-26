#pragma once

#include "IQtScreenController.h"
#include "ui/models/ServerListModel.h"

#include <QObject>
#include <QString>

class ServerListScreen;

/**
 * @brief Qt controller for the server browser screen.
 *
 * Owns a ServerListModel and syncs it from ServerListScreen::get_servers()
 * each event-loop tick.  Exposes Q_INVOKABLEs so QML can initiate connections.
 */
class ServerListController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(ServerListModel* model READ model CONSTANT)

  public:
    explicit ServerListController(QObject* parent = nullptr);

    /// IQtScreenController
    void sync(Screen& screen) override;

    ServerListModel* model() { return &m_model; }

    /// Connect to the server at the given index in the model.
    Q_INVOKABLE void connectToServer(int index);

    /// Connect directly to host:port without going through the server list.
    Q_INVOKABLE void directConnect(const QString& host, quint16 port);

  private:
    ServerListModel   m_model;
    ServerListScreen* m_screen = nullptr; ///< Cached from last sync(); valid while active.
};
