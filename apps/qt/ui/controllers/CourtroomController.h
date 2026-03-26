#pragma once

#include "IQtScreenController.h"
#include "ui/models/ChatModel.h"
#include "ui/models/EvidenceModel.h"
#include "ui/models/MusicAreaModel.h"
#include "ui/models/PlayerListModel.h"

#include <QObject>
#include <QString>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Qt controller for the courtroom screen.
 *
 * Drives four Qt models from server events consumed on each sync() tick:
 *   - ChatModel        (OOC chat log, CT packets)
 *   - PlayerListModel  (player roster, PR/PU packets via PlayerListEvent)
 *   - EvidenceModel    (evidence list, LE packets)
 *   - MusicAreaModel   (areas + music tracks, SM/FA/FM + ARUP packets)
 *
 * Also exposes health bars, character name, and now-playing text as
 * Q_PROPERTYs for QML binding.
 */
class CourtroomController : public IQtScreenController {
    Q_OBJECT

    Q_PROPERTY(ChatModel*       chatModel       READ chatModel       CONSTANT)
    Q_PROPERTY(PlayerListModel* playerListModel READ playerListModel CONSTANT)
    Q_PROPERTY(EvidenceModel*   evidenceModel   READ evidenceModel   CONSTANT)
    Q_PROPERTY(MusicAreaModel*  musicAreaModel  READ musicAreaModel  CONSTANT)

    Q_PROPERTY(int     defHp       READ defHp       NOTIFY defHpChanged)
    Q_PROPERTY(int     proHp       READ proHp       NOTIFY proHpChanged)
    Q_PROPERTY(QString nowPlaying  READ nowPlaying  NOTIFY nowPlayingChanged)
    Q_PROPERTY(QString charName    READ charName    NOTIFY charNameChanged)

  public:
    explicit CourtroomController(QObject* parent = nullptr);

    void sync(Screen& screen) override;

    ChatModel*       chatModel()       { return &m_chat;       }
    PlayerListModel* playerListModel() { return &m_players;    }
    EvidenceModel*   evidenceModel()   { return &m_evidence;   }
    MusicAreaModel*  musicAreaModel()  { return &m_musicArea;  }

    int     defHp()      const { return m_defHp;      }
    int     proHp()      const { return m_proHp;      }
    QString nowPlaying() const { return m_nowPlaying;  }
    QString charName()   const { return m_charName;    }

    /// Reset all models and properties (called on disconnect / POP_TO_ROOT).
    Q_INVOKABLE void reset();

    /// Disconnect and return to the server list.
    Q_INVOKABLE void disconnect();

  signals:
    void defHpChanged();
    void proHpChanged();
    void nowPlayingChanged();
    void charNameChanged();

  private:
    void drainChat();
    void drainPlayerList();
    void drainEvidence();
    void drainMusicList();
    void drainAreaUpdates();
    void drainHealthBars();
    void drainNowPlaying();

    // Models
    ChatModel        m_chat;
    PlayerListModel  m_players;
    EvidenceModel    m_evidence;
    MusicAreaModel   m_musicArea;

    // Scalar state properties
    int     m_defHp      = 0;
    int     m_proHp      = 0;
    QString m_nowPlaying;
    QString m_charName;

    // Local state for incremental area/track builds
    struct AreaState {
        std::string name;
        int         playerCount = -1;
        std::string status      = "Unknown";
        std::string cm          = "Unknown";
        std::string lock        = "Unknown";
    };
    std::vector<AreaState>  m_areas;
    std::vector<std::string> m_tracks;

    // Player cache for incremental PR/PU events
    struct PlayerCacheEntry {
        QString name;
        QString character;
        int     areaId = -1;
    };
    std::map<int, PlayerCacheEntry> m_playerCache;

    void rebuildMusicAreaModel();
    void rebuildPlayerModel();
};
