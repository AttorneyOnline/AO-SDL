#pragma once

#include <QAbstractListModel>
#include <QString>

#include <vector>

/**
 * @brief Flat list model for the character roster on the char-select screen.
 *
 * Exposes the character folder name and taken status.
 * Populated by CharSelectController::drain() from CharacterListEvent /
 * CharsCheckEvent — no CharSelectScreen middleman.
 */
class CharListModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role { NameRole = Qt::UserRole + 1, TakenRole, IconSourceRole };

    struct CharEntry {
        QString name;
        bool    taken      = false;
        QString iconSource;
    };

    explicit CharListModel(QObject* parent = nullptr);

    /// Remove all entries (emits beginResetModel/endResetModel).
    void clear();

    /// Append a batch of entries using beginInsertRows/endInsertRows.
    void appendBatch(const std::vector<CharEntry>& batch);

    /// Update the taken flag for a single row (emits dataChanged).
    void setTaken(int row, bool taken);

    void setIconSource(int row, const QString& source);

    int     rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<CharEntry> m_entries;
};
