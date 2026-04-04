#pragma once

#include "event/EvidenceListEvent.h"

#include <QAbstractListModel>
#include <QString>

#include <vector>

/**
 * @brief QML model for the courtroom evidence list.
 *
 * Populated by CourtroomController when LE (evidence list) events arrive.
 * Roles: name, description, image.
 */
class EvidenceModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        ImageRole,
    };

    explicit EvidenceModel(QObject* parent = nullptr);

    /// Replace all evidence entries.
    void reset(const std::vector<EvidenceItem>& items);

    /// Clear all entries (e.g. on disconnect).
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<EvidenceItem> m_entries;
};
