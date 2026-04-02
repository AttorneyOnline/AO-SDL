#pragma once

#include <QAbstractListModel>
#include <QString>

#include <vector>

/**
 * @brief QML model for the emote selector grid.
 *
 * Populated by CourtroomController::applyCharacterData() when the character
 * sheet finishes loading.  Roles: comment, iconSource.
 * iconSource is initially empty; it resolves to "image://emoteicon/<char>/<i>"
 * once the icon has been confirmed present in the asset library.
 */
class EmoteModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role {
        CommentRole    = Qt::UserRole + 1,
        IconSourceRole,
    };

    struct Entry {
        QString comment;
        QString iconSource;
    };

    explicit EmoteModel(QObject* parent = nullptr);

    /// Replace the entire model contents.
    void reset(std::vector<Entry> entries);

    /// Clear all entries (e.g. on disconnect).
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<Entry> m_entries;
};
