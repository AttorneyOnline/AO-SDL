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
    enum Role { NameRole = Qt::UserRole + 1, TakenRole };

    struct CharEntry {
        QString name;
        bool    taken = false;
    };

    explicit CharListModel(QObject* parent = nullptr);

    void reset(const std::vector<CharEntry>& chars);

    int     rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<CharEntry> m_entries;
};
