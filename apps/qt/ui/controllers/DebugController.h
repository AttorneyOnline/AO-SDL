#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <chrono>

/// Controller for the debug overlay (F12).
///
/// Polled each drain() tick by the EngineEventBridge.  Reads engine stats
/// from QtDebugContext, EventManager, MediaManager and exposes them as
/// Q_PROPERTYs for QML binding.
class DebugController : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DebugController)

    // -- Performance --
    Q_PROPERTY(float fps READ fps NOTIFY statsChanged)
    Q_PROPERTY(float gameTickMs READ game_tick_ms NOTIFY statsChanged)
    Q_PROPERTY(float tickRateHz READ tick_rate_hz NOTIFY statsChanged)

    // -- Renderer --
    Q_PROPERTY(QString backendName READ backend_name CONSTANT)
    Q_PROPERTY(int internalScale READ internal_scale WRITE set_internal_scale NOTIFY internalScaleChanged)
    Q_PROPERTY(bool wireframe READ wireframe WRITE set_wireframe NOTIFY wireframeChanged)

    // -- Connection --
    Q_PROPERTY(QString connState READ conn_state NOTIFY statsChanged)
    Q_PROPERTY(QString serverInfo READ server_info NOTIFY statsChanged)
    Q_PROPERTY(int currentPlayers READ current_players NOTIFY statsChanged)
    Q_PROPERTY(int maxPlayers READ max_players NOTIFY statsChanged)

    // -- HTTP Streaming --
    Q_PROPERTY(int httpPending READ http_pending NOTIFY statsChanged)
    Q_PROPERTY(int httpCached READ http_cached NOTIFY statsChanged)
    Q_PROPERTY(int httpFailed READ http_failed NOTIFY statsChanged)
    Q_PROPERTY(int httpPoolPending READ http_pool_pending NOTIFY statsChanged)
    Q_PROPERTY(QString httpCachedSize READ http_cached_size NOTIFY statsChanged)

    // -- Asset Cache --
    Q_PROPERTY(int cacheEntries READ cache_entries NOTIFY statsChanged)
    Q_PROPERTY(QString cacheUsed READ cache_used NOTIFY statsChanged)
    Q_PROPERTY(QString cacheMax READ cache_max NOTIFY statsChanged)

    // -- Event Stats --
    Q_PROPERTY(QVariantList eventStats READ event_stats NOTIFY statsChanged)

  public:
    explicit DebugController(QObject* parent = nullptr);
    ~DebugController() override;

    /// Called by EngineEventBridge each tick.
    void drain();

    // -- Performance --
    float fps() const {
        return fps_;
    }
    float game_tick_ms() const {
        return game_tick_ms_;
    }
    float tick_rate_hz() const {
        return tick_rate_hz_;
    }

    // -- Renderer --
    QString backend_name() const;
    int internal_scale() const;
    void set_internal_scale(int scale);
    bool wireframe() const;
    void set_wireframe(bool on);

    // -- Connection --
    QString conn_state() const {
        return conn_state_;
    }
    QString server_info() const {
        return server_info_;
    }
    int current_players() const {
        return current_players_;
    }
    int max_players() const {
        return max_players_;
    }

    // -- HTTP Streaming --
    int http_pending() const {
        return http_pending_;
    }
    int http_cached() const {
        return http_cached_;
    }
    int http_failed() const {
        return http_failed_;
    }
    int http_pool_pending() const {
        return http_pool_pending_;
    }
    QString http_cached_size() const {
        return http_cached_size_;
    }

    // -- Asset Cache --
    int cache_entries() const {
        return cache_entries_;
    }
    QString cache_used() const {
        return cache_used_;
    }
    QString cache_max() const {
        return cache_max_;
    }

    // -- Event Stats --
    QVariantList event_stats() const {
        return event_stats_;
    }

  signals:
    void statsChanged();
    void internalScaleChanged();
    void wireframeChanged();

  private:
    static QString format_bytes(size_t bytes);

    // Performance
    float fps_ = 0.0f;
    float game_tick_ms_ = 0.0f;
    float tick_rate_hz_ = 0.0f;
    std::chrono::steady_clock::time_point last_drain_;
    int frame_count_ = 0;
    float fps_accum_ = 0.0f;

    // Connection
    QString conn_state_ = "Disconnected";
    QString server_info_;
    int current_players_ = 0;
    int max_players_ = 0;
    bool has_connection_ = false;

    // HTTP
    int http_pending_ = 0;
    int http_cached_ = 0;
    int http_failed_ = 0;
    int http_pool_pending_ = 0;
    QString http_cached_size_;

    // Cache
    int cache_entries_ = 0;
    QString cache_used_;
    QString cache_max_;

    // Event Stats
    QVariantList event_stats_;
    std::chrono::steady_clock::time_point last_event_snapshot_;
};
