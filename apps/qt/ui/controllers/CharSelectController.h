#pragma once

#include "IQtScreenController.h"
#include "ui/models/CharListModel.h"

#include <QAbstractListModel>
#include <QObject>
#include <QSortFilterProxyModel>
#include <QString>

#include <string>
#include <vector>

class ICController;
class UIManager;

/**
 * @brief Proxy that filters CharListModel rows by name text and/or taken status.
 *
 * setFilter() matches case-insensitively against the character folder name.
 * setHideTaken() removes rows whose TakenRole is true.
 */
class CharListFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit CharListFilterProxy(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setFilter(const QString& text) {
        m_filter = text.trimmed();
        invalidateFilter();
    }

    void setHideTaken(bool hide) {
        m_hideTaken = hide;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        if (m_hideTaken) {
            if (sourceModel()->data(idx, CharListModel::TakenRole).toBool())
                return false;
        }
        if (!m_filter.isEmpty()) {
            const QString name = sourceModel()->data(idx, CharListModel::NameRole).toString();
            return name.contains(m_filter, Qt::CaseInsensitive);
        }
        return true;
    }

private:
    QString m_filter;
    bool m_hideTaken = false;
};

/**
 * @brief Qt controller for the character selection screen.
 *
 * drain() consumes CharacterListEvent, CharsCheckEvent, and UIEvent directly
 * from EventManager.  No CharSelectScreen is needed as a data source.
 *
 * On UIEvent::ENTERED_COURTROOM the controller:
 *   1. Injects the character name into ICController.
 *   2. Pushes CourtroomScreen onto UIManager's stack.
 *
 * The character list is fed into the QML model in batches
 * (kHydrateBatch entries per tick) so the UI hydrates progressively
 * rather than stalling on a single frame.
 *
 * filteredModel wraps the raw model with a CharListFilterProxy; QML binds
 * to filteredModel.  selectCharacter() accepts a proxy-model row and maps
 * it back to the source index before publishing CharSelectRequestEvent.
 */
class CharSelectController : public IQtScreenController {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* filteredModel READ filtered_model CONSTANT)

  public:
    explicit CharSelectController(UIManager& uiMgr, ICController& icCtrl, QObject* parent = nullptr);

    /// IQtScreenController
    void drain() override;

    QAbstractItemModel* filtered_model() { return &m_filterProxy; }

    /// Update the name search filter.  Empty string shows all characters.
    Q_INVOKABLE void setFilter(const QString& text) { m_filterProxy.setFilter(text); }

    /// Show or hide taken characters.
    Q_INVOKABLE void setHideTaken(bool hide) { m_filterProxy.setHideTaken(hide); }

    /// Select the character at the given proxy-model row.  Publishes CharSelectRequestEvent.
    Q_INVOKABLE void selectCharacter(int proxyRow);

    /// Return to the server list.
    Q_INVOKABLE void disconnect();

  private:
    struct CharEntry {
        std::string folder;
        bool taken = false;
    };

    void hydrateModel();

    UIManager& m_uiMgr;
    ICController& m_icCtrl;
    CharListModel m_model;
    CharListFilterProxy m_filterProxy;

    std::vector<CharEntry> m_chars;
    int m_selected = -1;      ///< Source-model index of the last-clicked character.
    int m_hydrateCursor = 0;

    /// Entries inserted into the model per tick during initial hydration.
    static constexpr int kHydrateBatch = 64;
};
