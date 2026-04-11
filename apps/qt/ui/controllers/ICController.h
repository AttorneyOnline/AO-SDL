#pragma once

#include "IQtScreenController.h"
#include "ui/models/EmoteModel.h"
#include "ui/models/ICLogModel.h"

#include <QObject>
#include <QString>

#include <string>

class UIManager;

/**
 * @brief Qt controller for IC (in-character) message composition.
 *
 * Manages character identity (charName, showname, side), the emote grid,
 * and all IC composition flags (flip, preAnim, additive, noInterrupt,
 * objectionMod, effectMod).
 *
 * drain() polls the active CourtroomScreen for completed character sheet
 * loading via UIManager — identical pattern to SDL's apply_character_data().
 *
 * sendICMessage() builds an ICMessageData from current state and publishes
 * OutgoingICMessageEvent. selectEmote() auto-updates the preAnim flag from
 * the character sheet.
 */
class ICController : public IQtScreenController {
    Q_OBJECT

    Q_PROPERTY(EmoteModel* emoteModel READ emoteModel CONSTANT)
    Q_PROPERTY(ICLogModel* icLogModel READ icLogModel CONSTANT)
    Q_PROPERTY(QString charName READ charName NOTIFY charNameChanged)
    Q_PROPERTY(QString showname READ showname WRITE setShowname NOTIFY shownameChanged)
    Q_PROPERTY(QString side READ side NOTIFY sideChanged)
    Q_PROPERTY(int selectedEmote READ selectedEmote NOTIFY selectedEmoteChanged)
    Q_PROPERTY(bool preAnim READ preAnim WRITE setPreAnim NOTIFY preAnimChanged)
    Q_PROPERTY(bool flip READ flip WRITE setFlip NOTIFY flipChanged)
    Q_PROPERTY(bool noInterrupt READ noInterrupt WRITE setNoInterrupt NOTIFY noInterruptChanged)
    Q_PROPERTY(bool additive READ additive WRITE setAdditive NOTIFY additiveChanged)
    Q_PROPERTY(int objectionMod READ objectionMod WRITE setObjectionMod NOTIFY objectionModChanged)
    Q_PROPERTY(int effectMod READ effectMod WRITE setEffectMod NOTIFY effectModChanged)

  public:
    explicit ICController(UIManager& uiMgr, QObject* parent = nullptr);

    void drain() override;

    EmoteModel* emoteModel() {
        return &m_emotes;
    }
    ICLogModel* icLogModel() {
        return &m_icLog;
    }

    // --- Property accessors ---
    QString charName() const {
        return m_charName;
    }
    QString showname() const {
        return m_showname;
    }
    QString side() const {
        return m_side;
    }
    int selectedEmote() const {
        return m_selectedEmote;
    }
    bool preAnim() const {
        return m_preAnim;
    }
    bool flip() const {
        return m_flip;
    }
    bool noInterrupt() const {
        return m_noInterrupt;
    }
    bool additive() const {
        return m_additive;
    }
    int objectionMod() const {
        return m_objectionMod;
    }
    int effectMod() const {
        return m_effectMod;
    }

    // --- Setters (QML write-back) ---
    void setShowname(const QString& v);
    void setPreAnim(bool v);
    void setFlip(bool v);
    void setNoInterrupt(bool v);
    void setAdditive(bool v);
    void setObjectionMod(int v);
    void setEffectMod(int v);

    /**
     * @brief Called by CharSelectController before pushing CourtroomScreen.
     *
     * Sets charName before the screen is visible so QML evaluates it correctly
     * on first render.
     */
    void setInitialCharName(const std::string& name);

    /**
     * @brief Set the active side and notify QML.
     * Side strings: "def", "pro", "wit", "jud", "hld", "hlp".
     */
    Q_INVOKABLE void selectSide(const QString& side);

    /**
     * @brief Select emote at index and auto-update preAnim from the character sheet.
     */
    Q_INVOKABLE void selectEmote(int index);

    /**
     * @brief Publish an OutgoingICMessageEvent with current composition state.
     * @param message IC message text.
     */
    Q_INVOKABLE void sendICMessage(const QString& message);

    /// Reset all composition state (called on disconnect).
    void reset();

  signals:
    void charNameChanged();
    void shownameChanged();
    void sideChanged();
    void selectedEmoteChanged();
    void preAnimChanged();
    void flipChanged();
    void noInterruptChanged();
    void additiveChanged();
    void objectionModChanged();
    void effectModChanged();

  private:
    void drainICLog();

    /**
     * @brief Poll the active CourtroomScreen for completed character loading.
     * Called every drain() tick.
     */
    void applyCharacterData();

    EmoteModel m_emotes;
    ICLogModel m_icLog;

    QString m_charName;
    QString m_showname;
    QString m_side;
    int m_charId = -1;
    int m_selectedEmote = 0;
    bool m_preAnim = false;
    bool m_flip = false;
    bool m_noInterrupt = false;
    bool m_additive = false;
    int m_objectionMod = 0;
    int m_effectMod = 0;

    // Tracks character-sheet load generation to avoid redundant rebuilds.
    int m_lastLoadGen = -1;

    UIManager& m_uiMgr;
};
