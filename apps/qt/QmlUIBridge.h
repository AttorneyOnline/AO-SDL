#pragma once

#include <QAbstractListModel>
#include <QImage>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <chrono>
#include <map>
#include <memory>

class UIManager;
struct ServerEntry;
class ICharacterSheet;
struct EvidenceItem;

// --- ServerListModel ---

class ServerListModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Roles { NameRole = Qt::UserRole + 1, PlayersRole, DescriptionRole, HasWsRole };

    explicit ServerListModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void update_from(const std::vector<ServerEntry>& servers);

  private:
    struct Entry {
        QString name;
        int players;
        QString description;
        bool has_ws;
    };
    QVector<Entry> entries_;
};

// --- CharListModel ---

class CharListModel : public QAbstractListModel {
    Q_OBJECT

  public:
    enum Roles {
        FolderRole = Qt::UserRole + 1,
        HasIconRole,
        TakenRole,
    };

    explicit CharListModel(QObject* parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reset(int count, const std::function<QString(int)>& folder_fn);

    struct CharSnapshot {
        bool taken;
        bool has_icon;
        QImage image;
    };
    void refresh_status(int count, const std::function<CharSnapshot(int)>& snapshot_fn);

    QImage icon_image(int index) const;

  private:
    struct Entry {
        QString folder;
        bool has_icon;
        bool taken;
        QImage icon;
    };
    QVector<Entry> entries_;
};

// --- QmlUIBridge ---

class QmlUIBridge : public QObject {
    Q_OBJECT

    // Screen navigation
    Q_PROPERTY(QString activeScreenId READ active_screen_id NOTIFY active_screen_id_changed)

    // Server list
    Q_PROPERTY(ServerListModel* serverListModel READ server_list_model CONSTANT)
    Q_PROPERTY(int selectedServer READ selected_server NOTIFY selected_server_changed)

    // Character select
    Q_PROPERTY(CharListModel* charListModel READ char_list_model CONSTANT)
    Q_PROPERTY(int selectedChar READ selected_char NOTIFY selected_char_changed)
    Q_PROPERTY(int charCount READ char_count NOTIFY char_count_changed)

    // Courtroom — general
    Q_PROPERTY(QString characterName READ character_name NOTIFY character_name_changed)
    Q_PROPERTY(bool courtroomLoading READ courtroom_loading NOTIFY courtroom_loading_changed)

    // IC message state (bidirectional with QML)
    Q_PROPERTY(QString showname READ showname WRITE set_showname NOTIFY showname_changed)
    Q_PROPERTY(int selectedEmote READ selected_emote_idx WRITE set_selected_emote NOTIFY selected_emote_changed)
    Q_PROPERTY(int objectionMod READ objection_mod WRITE set_objection_mod NOTIFY objection_mod_changed)
    Q_PROPERTY(int sideIndex READ side_index WRITE set_side_index NOTIFY side_index_changed)
    Q_PROPERTY(int textColor READ text_color WRITE set_text_color NOTIFY text_color_changed)
    Q_PROPERTY(bool flip READ flip WRITE set_flip NOTIFY flip_changed)
    Q_PROPERTY(bool preAnim READ pre_anim WRITE set_pre_anim NOTIFY pre_anim_changed)
    Q_PROPERTY(bool realizationEffect READ realization WRITE set_realization NOTIFY realization_changed)
    Q_PROPERTY(bool screenshakeEffect READ screenshake WRITE set_screenshake NOTIFY screenshake_changed)
    Q_PROPERTY(bool additiveText READ additive WRITE set_additive NOTIFY additive_changed)

    // Courtroom data (read-only from QML)
    Q_PROPERTY(QString icLog READ ic_log NOTIFY ic_log_changed)
    Q_PROPERTY(QString oocLog READ ooc_log NOTIFY ooc_log_changed)
    Q_PROPERTY(QString nowPlaying READ now_playing NOTIFY now_playing_changed)
    Q_PROPERTY(int defHp READ def_hp NOTIFY hp_changed)
    Q_PROPERTY(int proHp READ pro_hp NOTIFY hp_changed)
    Q_PROPERTY(QString timerDisplay READ timer_display NOTIFY timer_display_changed)
    Q_PROPERTY(QVariantList musicTracks READ music_tracks NOTIFY music_tracks_changed)
    Q_PROPERTY(QVariantList areas READ areas_data NOTIFY areas_changed)
    Q_PROPERTY(QVariantList evidence READ evidence_data NOTIFY evidence_changed)
    Q_PROPERTY(QVariantList players READ players_data NOTIFY players_changed)
    Q_PROPERTY(QVariantList emotes READ emotes_data NOTIFY emotes_changed)
    Q_PROPERTY(int emoteIconGeneration READ emote_icon_generation NOTIFY emote_icons_refreshed)

  public:
    explicit QmlUIBridge(UIManager& ui_mgr, QObject* parent = nullptr);

    /// Called each frame (on frameSwapped) to sync engine → QML state.
    void sync_from_engine();

    // --- Screen navigation ---
    QString active_screen_id() const;
    ServerListModel* server_list_model();
    int selected_server() const;
    CharListModel* char_list_model();
    int selected_char() const;
    int char_count() const;
    QString character_name() const;
    bool courtroom_loading() const;

    // --- IC message state ---
    QString showname() const;
    void set_showname(const QString& v);
    int selected_emote_idx() const;
    void set_selected_emote(int v);
    int objection_mod() const;
    void set_objection_mod(int v);
    int side_index() const;
    void set_side_index(int v);
    int text_color() const;
    void set_text_color(int v);
    bool flip() const;
    void set_flip(bool v);
    bool pre_anim() const;
    void set_pre_anim(bool v);
    bool realization() const;
    void set_realization(bool v);
    bool screenshake() const;
    void set_screenshake(bool v);
    bool additive() const;
    void set_additive(bool v);

    // --- Courtroom data ---
    QString ic_log() const;
    QString ooc_log() const;
    QString now_playing() const;
    int def_hp() const;
    int pro_hp() const;
    QString timer_display() const;
    QVariantList music_tracks() const;
    QVariantList areas_data() const;
    QVariantList evidence_data() const;
    QVariantList players_data() const;
    QVariantList emotes_data() const;
    int emote_icon_generation() const;

    /// Retrieve cached emote icon QImage by index (for EmoteIconProvider).
    QImage emote_icon_image(int index) const;

    // --- Actions (Q_INVOKABLE) ---
    Q_INVOKABLE void select_server(int index);
    Q_INVOKABLE void direct_connect(const QString& host, int port);
    Q_INVOKABLE void select_character(int index);
    Q_INVOKABLE void request_disconnect();
    Q_INVOKABLE void pop_screen();
    Q_INVOKABLE void send_ic_message(const QString& message);
    Q_INVOKABLE void send_ooc_message(const QString& name, const QString& message);
    Q_INVOKABLE void play_music(const QString& track);
    Q_INVOKABLE void set_health(int side, int value);
    Q_INVOKABLE void set_volume(int category, float value);
    Q_INVOKABLE void select_emote(int index);

  signals:
    void active_screen_id_changed();
    void selected_server_changed();
    void selected_char_changed();
    void char_count_changed();
    void character_name_changed();
    void courtroom_loading_changed();

    void showname_changed();
    void selected_emote_changed();
    void objection_mod_changed();
    void side_index_changed();
    void text_color_changed();
    void flip_changed();
    void pre_anim_changed();
    void realization_changed();
    void screenshake_changed();
    void additive_changed();

    void ic_log_changed();
    void ooc_log_changed();
    void now_playing_changed();
    void hp_changed();
    void timer_display_changed();
    void music_tracks_changed();
    void areas_changed();
    void evidence_changed();
    void players_changed();
    void emotes_changed();
    void emote_icons_refreshed();

  private:
    void poll_courtroom_events();
    void sync_courtroom_character_data();
    void refresh_emote_icons();
    void update_timer_display();
    void rebuild_players_list();

    UIManager& ui_mgr_;

    // --- Screen state ---
    QString cached_screen_id_;
    ServerListModel server_model_{this};
    CharListModel char_model_{this};
    int cached_server_count_ = 0;
    int cached_selected_server_ = -1;
    int cached_char_count_ = 0;
    int cached_selected_char_ = -1;
    QString cached_character_name_;
    bool cached_courtroom_loading_ = false;

    // --- IC message state ---
    QString ic_showname_;
    int ic_selected_emote_ = 0;
    int ic_objection_mod_ = 0;
    int ic_side_index_ = 0;
    int ic_text_color_ = 0;
    bool ic_flip_ = false;
    bool ic_pre_anim_ = false;
    bool ic_realization_ = false;
    bool ic_screenshake_ = false;
    bool ic_additive_ = false;

    // --- Courtroom data ---
    QString ic_log_;
    QString ooc_log_;
    QString now_playing_;
    int def_hp_ = 0;
    int pro_hp_ = 0;

    // Music tracks
    QVariantList cached_music_tracks_;
    // Area list
    struct AreaEntry {
        QString name;
        int players = -1;
        QString status;
        QString cm;
        bool locked = false;
    };
    std::vector<AreaEntry> area_entries_;
    QVariantList cached_areas_;

    // Evidence
    QVariantList cached_evidence_;

    // Players
    struct PlayerInfo {
        QString name;
        QString character;
        QString charname;
        int area_id = -1;
    };
    std::map<int, PlayerInfo> players_map_;
    QVariantList cached_players_;

    // Emotes
    QVariantList cached_emotes_;
    QVector<QImage> emote_icon_images_;
    int emote_icon_generation_ = 0;
    int cached_load_generation_ = -1;
    std::shared_ptr<ICharacterSheet> cached_char_sheet_;

    // Timers
    static constexpr int MAX_TIMERS = 5;
    struct TimerState {
        bool visible = false;
        bool running = false;
        int64_t remaining_ms = 0;
        std::chrono::steady_clock::time_point last_tick;
    };
    TimerState timers_[MAX_TIMERS] = {};
    QString cached_timer_display_;
};
