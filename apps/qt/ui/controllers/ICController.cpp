#include "ICController.h"

#include "ao/ui/screens/CourtroomScreen.h"
#include "event/EventManager.h"
#include "event/ICLogEvent.h"
#include "event/OutgoingICMessageEvent.h"
#include "ui/UIManager.h"
#include "utils/Log.h"

ICController::ICController(UIManager& uiMgr, QObject* parent) : IQtScreenController(parent), m_uiMgr(uiMgr) {
}

void ICController::drain() {
    drainICLog();
    applyCharacterData();
}

void ICController::drainICLog() {
    auto& ch = EventManager::instance().get_channel<ICLogEvent>();
    while (auto ev = ch.get_event()) {
        m_icLog.appendEntry({QString::fromStdString(ev->get_showname()), QString::fromStdString(ev->get_message()),
                             ev->get_color_idx()});
    }
}

// --------------------------------------------------------------------------
// Setters
// --------------------------------------------------------------------------

void ICController::setInitialCharName(const std::string& name) {
    QString qname = QString::fromStdString(name);
    if (qname == m_charName)
        return;
    m_charName = qname;
    emit charNameChanged();
}

void ICController::setShowname(const QString& v) {
    if (v == m_showname)
        return;
    m_showname = v;
    emit shownameChanged();
}

void ICController::setPreAnim(bool v) {
    if (v == m_preAnim)
        return;
    m_preAnim = v;
    emit preAnimChanged();
}

void ICController::setFlip(bool v) {
    if (v == m_flip)
        return;
    m_flip = v;
    emit flipChanged();
}

void ICController::setNoInterrupt(bool v) {
    if (v == m_noInterrupt)
        return;
    m_noInterrupt = v;
    emit noInterruptChanged();
}

void ICController::setAdditive(bool v) {
    if (v == m_additive)
        return;
    m_additive = v;
    emit additiveChanged();
}

void ICController::setObjectionMod(int v) {
    if (v == m_objectionMod)
        return;
    m_objectionMod = v;
    emit objectionModChanged();
}

void ICController::setEffectMod(int v) {
    if (v == m_effectMod)
        return;
    m_effectMod = v;
    emit effectModChanged();
}

// --------------------------------------------------------------------------
// Invokables
// --------------------------------------------------------------------------

void ICController::selectSide(const QString& side) {
    if (side == m_side)
        return;
    m_side = side;
    emit sideChanged();
}

void ICController::selectEmote(int index) {
    if (index < 0 || index >= m_emotes.rowCount())
        return;
    if (m_selectedEmote != index) {
        m_selectedEmote = index;
        emit selectedEmoteChanged();
    }

    // Auto-update preAnim from the selected emote's character sheet entry.
    auto* screen = dynamic_cast<CourtroomScreen*>(m_uiMgr.active_screen());
    if (!screen)
        return;
    auto sheet = screen->get_character_sheet();
    if (sheet && index < sheet->emote_count()) {
        const auto& emo = sheet->emote(index);
        setPreAnim(!emo.pre_anim.empty() && emo.pre_anim != "-");
    }
}

void ICController::sendICMessage(const QString& message) {
    ICMessageData data;
    data.character = m_charName.toStdString();
    data.char_id = m_charId;
    data.message = message.toStdString();
    data.showname = m_showname.toStdString();
    data.side = m_side.isEmpty() ? "def" : m_side.toStdString();
    data.objection_mod = m_objectionMod;
    data.flip = m_flip ? 1 : 0;
    data.additive = m_additive ? 1 : 0;

    auto* screen = dynamic_cast<CourtroomScreen*>(m_uiMgr.active_screen());
    if (screen) {
        auto sheet = screen->get_character_sheet();
        if (sheet && m_selectedEmote >= 0 && m_selectedEmote < sheet->emote_count()) {
            const auto& emo = sheet->emote(m_selectedEmote);
            data.emote = emo.anim_name;
            data.pre_emote = emo.pre_anim;
            data.desk_mod = emo.desk_mod;
            if (!emo.sfx_name.empty() && emo.sfx_name != "0") {
                data.sfx_name = emo.sfx_name;
                data.sfx_delay = emo.sfx_delay;
            }
        }
    }
    data.emote_mod = m_preAnim ? 1 : 0;

    EventManager::instance().get_channel<OutgoingICMessageEvent>().publish(OutgoingICMessageEvent(std::move(data)));
}

void ICController::reset() {
    Log::debug("[ICController] reset");
    m_emotes.clear();
    m_icLog.clear();
    if (!m_charName.isEmpty()) {
        m_charName.clear();
        emit charNameChanged();
    }
    if (!m_showname.isEmpty()) {
        m_showname.clear();
        emit shownameChanged();
    }
    if (!m_side.isEmpty()) {
        m_side.clear();
        emit sideChanged();
    }
    if (m_selectedEmote != 0) {
        m_selectedEmote = 0;
        emit selectedEmoteChanged();
    }
    if (m_preAnim) {
        m_preAnim = false;
        emit preAnimChanged();
    }
    if (m_flip) {
        m_flip = false;
        emit flipChanged();
    }
    if (m_noInterrupt) {
        m_noInterrupt = false;
        emit noInterruptChanged();
    }
    if (m_additive) {
        m_additive = false;
        emit additiveChanged();
    }
    if (m_objectionMod != 0) {
        m_objectionMod = 0;
        emit objectionModChanged();
    }
    if (m_effectMod != 0) {
        m_effectMod = 0;
        emit effectModChanged();
    }
    m_charId = -1;
    m_lastLoadGen = -1;
}

// --------------------------------------------------------------------------
// Character sheet polling
// --------------------------------------------------------------------------

void ICController::applyCharacterData() {
    auto* screen = dynamic_cast<CourtroomScreen*>(m_uiMgr.active_screen());
    if (!screen || screen->is_loading())
        return;
    if (screen->load_generation() == m_lastLoadGen)
        return;

    m_lastLoadGen = screen->load_generation();
    m_charId = screen->get_char_id();

    QString qname = QString::fromStdString(screen->get_character_name());
    if (qname != m_charName) {
        m_charName = qname;
        emit charNameChanged();
    }

    auto sheet = screen->get_character_sheet();
    if (sheet) {
        QString qside = QString::fromStdString(sheet->side());
        if (qside != m_side) {
            m_side = qside;
            emit sideChanged();
        }
        QString qshow = QString::fromStdString(sheet->showname());
        if (qshow != m_showname) {
            m_showname = qshow;
            emit shownameChanged();
        }
    }

    // Build emote entries; icons are served on-demand by EmoteIconProvider.
    std::vector<EmoteModel::Entry> entries;
    int emoteCount = sheet ? sheet->emote_count() : 0;
    entries.reserve(emoteCount);
    for (int i = 0; i < emoteCount; i++) {
        EmoteModel::Entry e;
        e.comment = QString::fromStdString(sheet->emote(i).comment);
        e.iconSource = QStringLiteral("image://emoteicon/%1/%2").arg(qname).arg(i);
        entries.push_back(std::move(e));
    }
    m_emotes.reset(std::move(entries));

    // Reset selection to first emote and auto-set preAnim.
    int newSel = 0;
    bool newPre = false;
    if (sheet && emoteCount > 0) {
        const auto& emo = sheet->emote(0);
        newPre = !emo.pre_anim.empty() && emo.pre_anim != "-";
    }
    if (m_selectedEmote != newSel) {
        m_selectedEmote = newSel;
        emit selectedEmoteChanged();
    }
    if (m_preAnim != newPre) {
        m_preAnim = newPre;
        emit preAnimChanged();
    }

    Log::debug("[ICController] character data applied: {} ({} emotes)", screen->get_character_name(), emoteCount);
}
