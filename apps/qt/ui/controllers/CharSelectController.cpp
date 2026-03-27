#include "CharSelectController.h"

#include "CourtroomController.h"
#include "ao/ui/screens/CourtroomScreen.h"
#include "asset/MediaManager.h"
#include "event/CharSelectRequestEvent.h"
#include "event/CharacterListEvent.h"
#include "event/CharsCheckEvent.h"
#include "event/EventManager.h"
#include "event/UIEvent.h"
#include "ui/UIManager.h"

#include <format>
#include <memory>

CharSelectController::CharSelectController(UIManager&           uiMgr,
                                           CourtroomController& crCtrl,
                                           QObject*             parent)
    : IQtScreenController(parent)
    , m_uiMgr(uiMgr)
    , m_crCtrl(crCtrl)
{}

void CharSelectController::drain() {
    bool modelDirty = false;

    // New character list: rebuild roster.
    auto& charListCh = EventManager::instance().get_channel<CharacterListEvent>();
    while (auto ev = charListCh.get_event()) {
        m_chars.clear();
        m_chars.reserve(ev->get_characters().size());
        for (const auto& folder : ev->get_characters())
            m_chars.push_back({folder, false});
        m_prefetchCursor = 0;
        m_retryCursor    = 0;
        m_selected       = -1;
        modelDirty       = true;
    }

    // Update taken flags.
    auto& checkCh = EventManager::instance().get_channel<CharsCheckEvent>();
    while (auto ev = checkCh.get_event()) {
        const auto& taken = ev->get_taken();
        for (size_t i = 0; i < taken.size() && i < m_chars.size(); ++i)
            m_chars[i].taken = taken[i];
        modelDirty = true;
    }

    if (modelDirty)
        syncModel();

    // Progressively prefetch character icons into the asset cache.
    // GPU upload is deferred to Phase 4 (requires QML image provider).
    prefetchIcons();

    // Navigation: server confirmed character selection → enter courtroom.
    auto& uiCh = EventManager::instance().get_channel<UIEvent>();
    while (auto ev = uiCh.get_event()) {
        if (ev->get_type() == UIEventType::ENTERED_COURTROOM) {
            // Inject character name into CourtroomController before pushing.
            m_crCtrl.setInitialCharName(ev->get_character_name());
            m_uiMgr.push_screen(
                std::make_unique<CourtroomScreen>(ev->get_character_name(),
                                                  ev->get_char_id()));
        }
    }
}

void CharSelectController::selectCharacter(int index) {
    if (index < 0 || index >= static_cast<int>(m_chars.size()))
        return;
    if (m_chars[index].taken && index != m_selected)
        return;

    if (index == m_selected) {
        // Already confirmed — push the courtroom directly without server round-trip.
        m_crCtrl.setInitialCharName(m_chars[index].folder);
        m_uiMgr.push_screen(
            std::make_unique<CourtroomScreen>(m_chars[index].folder, index));
        return;
    }

    m_selected = index;
    EventManager::instance()
        .get_channel<CharSelectRequestEvent>()
        .publish(CharSelectRequestEvent(index));
}

void CharSelectController::disconnect() {
    m_uiMgr.pop_to_root();
}

// --------------------------------------------------------------------------
// Private helpers
// --------------------------------------------------------------------------

void CharSelectController::prefetchIcons() {
    if (m_chars.empty())
        return;

    AssetLibrary& lib  = MediaManager::instance().assets();
    auto          exts = MediaManager::instance().mounts_ref().http_extensions(0);
    if (exts.empty())
        exts = {"webp", "apng", "gif", "png"};

    // Drip-feed HTTP prefetch requests — 32 per tick on the initial pass.
    for (int i = 0; i < 32 && m_prefetchCursor < static_cast<int>(m_chars.size());
         ++i, ++m_prefetchCursor)
    {
        std::string path =
            std::format("characters/{}/char_icon", m_chars[m_prefetchCursor].folder);
        lib.prefetch(path, exts, 0);
    }

    // Retry pass for icons that failed transiently on the initial sweep.
    if (m_prefetchCursor >= static_cast<int>(m_chars.size())) {
        if (m_retryCursor >= static_cast<int>(m_chars.size()))
            m_retryCursor = 0;
        for (int i = 0; i < 16 && m_retryCursor < static_cast<int>(m_chars.size());
             ++i, ++m_retryCursor)
        {
            std::string path =
                std::format("characters/{}/char_icon", m_chars[m_retryCursor].folder);
            // Prefetch unconditionally — AssetLibrary deduplicates in-flight requests.
            lib.prefetch(path, exts, 0);
        }
    }
}

void CharSelectController::syncModel() {
    std::vector<CharListModel::CharEntry> entries;
    entries.reserve(m_chars.size());
    for (const auto& c : m_chars)
        entries.push_back({QString::fromStdString(c.folder), c.taken});
    m_model.reset(entries);
}
