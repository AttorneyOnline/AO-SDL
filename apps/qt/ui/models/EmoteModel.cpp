#include "EmoteModel.h"

EmoteModel::EmoteModel(QObject* parent) : QAbstractListModel(parent) {
}

void EmoteModel::reset(std::vector<Entry> entries) {
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
}

void EmoteModel::clear() {
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

int EmoteModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant EmoteModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_entries.size()))
        return {};
    const auto& e = m_entries[index.row()];
    switch (role) {
    case CommentRole:
        return e.comment;
    case IconSourceRole:
        return e.iconSource;
    default:
        return {};
    }
}

QHash<int, QByteArray> EmoteModel::roleNames() const {
    return {
        {CommentRole, "comment"},
        {IconSourceRole, "iconSource"},
    };
}
