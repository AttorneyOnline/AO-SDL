#include "EvidenceModel.h"

EvidenceModel::EvidenceModel(QObject* parent) : QAbstractListModel(parent) {
}

void EvidenceModel::reset(const std::vector<EvidenceItem>& items) {
    beginResetModel();
    m_entries = items;
    endResetModel();
}

void EvidenceModel::clear() {
    if (m_entries.empty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

int EvidenceModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant EvidenceModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_entries.size()))
        return {};

    const EvidenceItem& e = m_entries[index.row()];
    switch (role) {
    case NameRole:
        return QString::fromStdString(e.name);
    case DescriptionRole:
        return QString::fromStdString(e.description);
    case ImageRole:
        return QString::fromStdString(e.image);
    default:
        return {};
    }
}

QHash<int, QByteArray> EvidenceModel::roleNames() const {
    return {
        {NameRole, "name"},
        {DescriptionRole, "description"},
        {ImageRole, "image"},
    };
}
