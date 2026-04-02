#pragma once

#include "IQtScreenController.h"
#include "ui/models/ChatModel.h"
#include "ui/models/EmoteModel.h"
#include "ui/models/EvidenceModel.h"
#include "ui/models/MusicAreaModel.h"
#include "ui/models/PlayerListModel.h"

#include <QObject>
#include <QString>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

class UIManager;

/**
 * @brief Qt controller for the courtroom screen.
 *
 * Drives five Qt models from server events consumed on each drain() tick:
 *   - ChatModel        (OOC chat log, CT packets)
 *   - PlayerListModel  (player roster, PR/PU packets via PlayerListEvent)
 *   - EvidenceModel    (evidence list, LE packets)
 *   - MusicAreaModel   (areas + music tracks, SM/FA/FM + ARUP packets)
 *   - EmoteModel       (character emote grid, populated from character sheet)
 *
 * Also exposes health bars, character name, now-playing text, showname,
 * selected emote, pre-anim flag, and side as Q_PROPERTYs for QML binding.
 *
 * On every drain() tick, applyCharacterData() polls the active CourtroomScreen
 * for completed async character-sheet loading, mirroring the SDL controller's
 * apply_character_data() pattern.
 */
class CourtroomController : public IQtScreenController {
    Q_OBJECT

    // --- Read-only models ---
    Q_PROPERTY(ChatModel*       chatModel       READ chatModel       CONSTANT)
    Q_PROPERTY(PlayerListModel* playerListModel READ playerListModel CONSTANT)
    Q_PROPERTY(EvidenceModel*   evidenceModel   READ evidenceModel   CONSTANT)
    Q_PROPERTY(MusicAreaModel*  musicAreaModel  READ musicAreaModel  CONSTANT)
    Q_PROPERTY(EmoteModel*      emoteModel      READ emoteModel      CONSTANT)

    // --- HUD scalars ---
    Q_PROPERTY(int     defHp       READ defHp       NOTIFY defHpChanged)
    Q_PROPERTY(int     proHp       READ proHp       NOTIFY proHpChanged)
    Q_PROPERTY(QString nowPlaying  READ nowPlaying  NOTIFY nowPlayingChanged)

    // --- IC message composition state ---
    Q_PROPERTY(QString charName      READ charName      NOTIFY charNameChanged)
    Q_PROPERTY(QString showname      READ showname      WRITE setShowname   NOTIFY shownameChanged)
    Q_PROPERTY(QString side          READ side          NOTIFY sideChanged)
    Q_PROPERTY(int     selectedEmote READ selectedEmote NOTIFY selectedEmoteChanged)
    Q_PROPERTY(bool    preAnim       READ preAnim       WRITE setPreAnim    NOTIFY preAnimChanged)

  public:
    explicit CourtroomController(UIManager& uiMgr, QObject* parent = nullptr);

    void drain() override;

    // --- Model accessors ---
    ChatModel*       chatModel()       { return &m_chat;      }
    PlayerListModel* playerListModel() { return &m_players;   }
    EvidenceModel*   evidenceModel()   { return &m_evidence;  }
    MusicAreaModel*  musicAreaModel()  { return &m_musicArea; }
    EmoteModel*      emoteModel()      { return &m_emotes;    }

    // --- HUD accessors ---
    int     defHp()      const { return m_defHp;     }
    int     proHp()      const { return m_proHp;     }
    QString nowPlaying() const { return m_nowPlaying; }

    // --- IC composition accessors ---
    QString charName()      const { return m_charName;      }
    QString showname()      const { return m_showname;      }
    QString side()          const { return m_side;          }
    int     selectedEmote() const { return m_selectedEmote; }
    bool    preAnim()       const { return m_preAnim;       }

    // --- Setters (QML write-back) ---
    void setShowname(const QString& v);
    void setPreAnim(bool v);

    /**
     * @brief Called by CharSelectController when ENTERED_COURTROOM fires.
     *
     * Sets the initial character name before the courtroom screen is visible
     * so QML bindings evaluate against the correct value on first render.
     */
    void setInitialCharName(const std::string& name);

    /// Reset all models and properties (called on disconnect / pop_to_root).
    Q_INVOKABLE void reset();

    /// Disconnect and return to the server list.
    Q_INVOKABLE void disconnect();

    /**
     * @brief Select emote at index and auto-update the pre-anim flag.
     * Mirrors SDL's EmoteSelectorWidget click handler.
     */
    Q_INVOKABLE void selectEmote(int index);

    /**
     * @brief Publish an OutgoingICMessageEvent with the current composition state.
     *
     * Reads the character sheet from the active CourtroomScreen to fill emote,
     * pre-emote, desk-mod, and SFX fields — identical to SDL's ICChatWidget::send().
     *
     * @param message   The IC message text typed by the user.
     * @param objectionMod  0=none, 1=Objection, 2=Hold It!, 3=Take That!
     */
    Q_INVOKABLE void sendICMessage(const QString& message, int objectionMod = 0);

  signals:
    void defHpChanged();
    void proHpChanged();
    void nowPlayingChanged();
    void charNameChanged();
    void shownameChanged();
    void sideChanged();
    void selectedEmoteChanged();
    void preAnimChanged();

  private:
    // --- Event drain helpers ---
    void drainChat();
    void drainPlayerList();
    void drainEvidence();
    void drainMusicList();
    void drainAreaUpdates();
    void drainHealthBars();
    void drainNowPlaying();

    /**
     * @brief Poll the active CourtroomScreen for completed character loading.
     *
     * Called every drain() tick.  Applies character sheet + emote icons once
     * loading finishes, mirroring SDL's CourtroomController::apply_character_data().
     */
    void applyCharacterData();

    // --- Models ---
    ChatModel       m_chat;
    PlayerListModel m_players;
    EvidenceModel   m_evidence;
    MusicAreaModel  m_musicArea;
    EmoteModel      m_emotes;

    // --- HUD state ---
    int     m_defHp     = 0;
    int     m_proHp     = 0;
    QString m_nowPlaying;

    // --- IC composition state ---
    QString m_charName;
    QString m_showname;
    QString m_side;
    int     m_charId       = -1;
    int     m_selectedEmote = 0;
    bool    m_preAnim       = false;

    // --- Character-sheet load tracking (mirrors SDL's last_load_gen_) ---
    int m_lastLoadGen = -1;

    // --- Area / track cache for incremental ARUP rebuilds ---
    struct AreaState {
        std::string name;
        int         playerCount = -1;
        std::string status      = "Unknown";
        std::string cm          = "Unknown";
        std::string lock        = "Unknown";
    };
    std::vector<AreaState>   m_areas;
    std::vector<std::string> m_tracks;

    // --- Player cache for incremental PR/PU events ---
    struct PlayerCacheEntry {
        QString name;
        QString character;
        int     areaId = -1;
    };
    std::map<int, PlayerCacheEntry> m_playerCache;

    void rebuildMusicAreaModel();
    void rebuildPlayerModel();

    UIManager& m_uiMgr;
};
