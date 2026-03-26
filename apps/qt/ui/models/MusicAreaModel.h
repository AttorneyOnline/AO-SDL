#pragma once

#include <QAbstractListModel>
#include <QString>

#include <string>
#include <vector>

/**
 * @brief QML model for the combined music-track / area list.
 *
 * Populated by CourtroomController from SM/FM/FA server packets.
 * Each row represents either a music track or a game area:
 *   - Areas carry a player count > 0 and isArea == true.
 *   - Music tracks carry isArea == false; playerCount is 0.
 *
 * Roles: name, playerCount, isArea, status, cm, lock.
 */
class MusicAreaModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role {
        NameRole        = Qt::UserRole + 1,
        PlayerCountRole,
        IsAreaRole,
        StatusRole,
        CmRole,
        LockRole,
    };

    struct Entry {
        QString name;
        int     playerCount = 0;
        bool    isArea      = false;
        QString status;
        QString cm;
        QString lock;
    };

    explicit MusicAreaModel(QObject* parent = nullptr);

    /// Replace all rows.
    void reset(const std::vector<Entry>& entries);

    /// Clear all rows (e.g. on disconnect).
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<Entry> m_entries;
};
