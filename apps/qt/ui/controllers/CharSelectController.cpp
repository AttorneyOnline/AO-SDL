#include "CharSelectController.h"

#include "ao/ui/screens/CharSelectScreen.h"
#include "ui/Screen.h"
#include "ui/IUIRenderer.h"

// --------------------------------------------------------------------------
// CharListModel
// --------------------------------------------------------------------------

CharListModel::CharListModel(QObject* parent)
    : QAbstractListModel(parent) {}

void CharListModel::reset(const std::vector<CharEntry>& chars) {
    beginResetModel();
    m_entries = chars;
    endResetModel();
}

int CharListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant CharListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0
        || index.row() >= static_cast<int>(m_entries.size()))
        return {};
    const CharEntry& e = m_entries[index.row()];
    switch (role) {
    case NameRole:  return e.name;
    case TakenRole: return e.taken;
    default:        return {};
    }
}

QHash<int, QByteArray> CharListModel::roleNames() const {
    return { { NameRole, "name" }, { TakenRole, "taken" } };
}

// --------------------------------------------------------------------------
// CharSelectController
// --------------------------------------------------------------------------

CharSelectController::CharSelectController(QObject* parent)
    : IQtScreenController(parent) {}

void CharSelectController::sync(Screen& screen) {
    m_screen = static_cast<CharSelectScreen*>(&screen);

    const auto& chars = m_screen->get_chars();
    if (static_cast<int>(chars.size()) != m_model.rowCount()) {
        std::vector<CharListModel::CharEntry> entries;
        entries.reserve(chars.size());
        for (const auto& c : chars)
            entries.push_back({ QString::fromStdString(c.folder), c.taken });

        m_model.reset(entries);
    }
}

void CharSelectController::selectCharacter(int index) {
    if (m_screen)
        m_screen->select_character(index);
}

void CharSelectController::disconnect() {
    emit navActionRequested(IUIRenderer::NavAction::POP_TO_ROOT);
}
