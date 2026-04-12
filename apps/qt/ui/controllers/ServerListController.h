#pragma once

#include "IQtScreenController.h"
#include "game/ServerList.h"
#include "ui/models/ServerListModel.h"

#include <QObject>
#include <QSortFilterProxyModel>
#include <QString>

#include <vector>

class UIManager;

/**
 * @brief Proxy that filters ServerListModel rows by a text query.
 *
 * Matches case-insensitively against both the server name and description so
 * that partial searches work across either field.  Call setFilter() to update
 * the query; invalidateFilter() notifies the view immediately.
 */
class ServerListFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ServerListFilterProxy(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setFilter(const QString& text) {
        m_filter = text.trimmed();
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        if (m_filter.isEmpty())
            return true;
        const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        const QString name = sourceModel()->data(idx, ServerListModel::NameRole).toString();
        const QString desc = sourceModel()->data(idx, ServerListModel::DescriptionRole).toString();
        return name.contains(m_filter, Qt::CaseInsensitive)
            || desc.contains(m_filter, Qt::CaseInsensitive);
    }

private:
    QString m_filter;
};

/**
 * @brief Qt controller for the server browser screen.
 *
 * drain() consumes ServerListEvent directly from EventManager and updates
 * ServerListModel — no Screen object is involved.
 *
 * filteredModel wraps the raw model with a ServerListFilterProxy; QML binds
 * to filteredModel and calls setFilter() as the user types.  connectToServer()
 * accepts a proxy-model row and maps it back to the underlying entry.
 *
 * connectToServer() and directConnect() publish ServerConnectEvent and push
 * CharSelectScreen onto UIManager's stack, making UIManager the single
 * authority over navigation.
 */
class ServerListController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* filteredModel READ filtered_model CONSTANT)

  public:
    explicit ServerListController(UIManager& uiMgr, QObject* parent = nullptr);

    /// IQtScreenController
    void drain() override;

    QAbstractItemModel* filtered_model() { return &m_filterProxy; }

    /// Update the search filter.  Empty string shows all servers.
    Q_INVOKABLE void setFilter(const QString& text) { m_filterProxy.setFilter(text); }

    /// Connect to the server at the given proxy-model row.
    Q_INVOKABLE void connectToServer(int proxyRow);

    /// Connect directly to host:port without going through the server list.
    Q_INVOKABLE void directConnect(const QString& host, quint16 port);

  private:
    void doConnect(const std::string& host, uint16_t port);

    UIManager& m_uiMgr;
    ServerListModel m_model;
    ServerListFilterProxy m_filterProxy;
    std::vector<ServerEntry> m_entries; ///< Mirrored from ServerListEvent for connectToServer().
};
