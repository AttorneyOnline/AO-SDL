#include "CharSelectController.h"

#include "ICController.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "event/CharSelectRequestEvent.h"
#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"
#include "ui/UIManager.h"

#include "utils/Log.h"

#include <memory>

CharSelectController::CharSelectController(UIManager& uiMgr, ICController& icCtrl, QObject* parent)
    : IQtScreenController(parent), m_uiMgr(uiMgr), m_icCtrl(icCtrl) {
}

void CharSelectController::drain() {
    auto& charListCh = EventManager::instance().get_channel<CharacterListEvent>();
    while (auto ev = charListCh.get_event()) {
        m_chars.clear();
        m_chars.reserve(ev->get_characters().size());
        for (const auto& folder : ev->get_characters())
            m_chars.push_back({folder, false});
        m_hydrateCursor = 0;
        m_selected = -1;
        m_model.clear();
        Log::info("[CharSelectController] received character list ({} characters)", m_chars.size());
    }

    auto& checkCh = EventManager::instance().get_channel<CharsCheckEvent>();
    while (auto ev = checkCh.get_event()) {
        const auto& taken = ev->get_taken();
        for (size_t i = 0; i < taken.size() && i < m_chars.size(); ++i) {
            m_chars[i].taken = taken[i];
            if (static_cast<int>(i) < m_hydrateCursor)
                m_model.setTaken(static_cast<int>(i), taken[i]);
        }
    }

    hydrateModel();

    auto& uiCh = EventManager::instance().get_channel<UIEvent>();
    while (auto ev = uiCh.get_event()) {
        if (ev->get_type() == UIEventType::ENTERED_COURTROOM) {
            Log::info("[CharSelectController] entering courtroom as '{}' (id={})", ev->get_character_name(),
                      ev->get_char_id());
            m_icCtrl.setInitialCharName(ev->get_character_name());
            m_uiMgr.push_screen(std::make_unique<CourtroomScreen>(ev->get_character_name(), ev->get_char_id()));
        }
    }
}

void CharSelectController::selectCharacter(int index) {
    if (index < 0 || index >= static_cast<int>(m_chars.size()))
        return;
    if (m_chars[index].taken && index != m_selected)
        return;

    Log::debug("[CharSelectController] selectCharacter index={} folder='{}'", index, m_chars[index].folder);

    if (index == m_selected) {
        // Already confirmed — push courtroom directly without a server round-trip.
        m_icCtrl.setInitialCharName(m_chars[index].folder);
        m_uiMgr.push_screen(std::make_unique<CourtroomScreen>(m_chars[index].folder, index));
        return;
    }

    m_selected = index;
    EventManager::instance().get_channel<CharSelectRequestEvent>().publish(CharSelectRequestEvent(index));
}

void CharSelectController::disconnect() {
    Log::info("[CharSelectController] disconnecting, returning to server list");
    m_uiMgr.pop_to_root();
}

void CharSelectController::hydrateModel() {
    if (m_hydrateCursor >= static_cast<int>(m_chars.size()))
        return;

    int end = std::min(m_hydrateCursor + kHydrateBatch, static_cast<int>(m_chars.size()));

    std::vector<CharListModel::CharEntry> batch;
    batch.reserve(end - m_hydrateCursor);
    for (int i = m_hydrateCursor; i < end; ++i) {
        const auto& c = m_chars[i];
        // Always set iconSource — the async CharIconProvider handles the loading
        // state and emits finished() when the asset is available.  QML shows the
        // letter fallback while Image.status !== Image.Ready.
        QString source = QStringLiteral("image://charicon/") + QString::fromStdString(c.folder);
        batch.push_back({QString::fromStdString(c.folder), c.taken, source});
    }

    m_model.appendBatch(batch);
    m_hydrateCursor = end;
}
