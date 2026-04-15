#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <array>
#include <chrono>
#include <string>
#include <unordered_map>

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

    // -- Tick breakdown (pie) --
    /// List of {name, us, avgUs} per profiled tick section.  Averages use a
    /// 30-sample ring buffer; QML renders a pie chart from avgUs.
    Q_PROPERTY(QVariantList tickSections READ tick_sections NOTIFY statsChanged)

    /// QSG per-phase timing slices (sync/render/present/gui_gap) with the same
    /// 30-sample rolling average as tickSections.  Populated from
    /// QtRenderProfiler, which samples QQuickWindow lifecycle signals on the
    /// render thread.
    Q_PROPERTY(QVariantList qtSections READ qt_sections NOTIFY statsChanged)

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

    /// Snapshot of cache entries (refreshed ~every 2s) for the cache preview
    /// panel.  Each element is a QVariantMap with keys: path, format, bytes,
    /// bytesFmt, useCount, width, height, frameCount.
    Q_PROPERTY(QVariantList cacheList READ cache_list NOTIFY cacheListChanged)

    /// Virtual path of the entry selected for preview.  Writing here triggers
    /// a resolve of width/height/frameCount via peek() and makes the preview
    /// available through "image://cachepreview/<path>".
    Q_PROPERTY(QString selectedCachePath READ selected_cache_path WRITE set_selected_cache_path NOTIFY selectedCachePathChanged)
    Q_PROPERTY(int selectedCacheWidth READ selected_cache_width NOTIFY selectedCachePathChanged)
    Q_PROPERTY(int selectedCacheHeight READ selected_cache_height NOTIFY selectedCachePathChanged)
    Q_PROPERTY(int selectedCacheFrameCount READ selected_cache_frame_count NOTIFY selectedCachePathChanged)
    Q_PROPERTY(QString selectedCacheFormat READ selected_cache_format NOTIFY selectedCachePathChanged)

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

    QVariantList tick_sections() const {
        return tick_sections_;
    }

    QVariantList qt_sections() const {
        return qt_sections_;
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

    QVariantList cache_list() const {
        return cache_list_;
    }

    QString selected_cache_path() const {
        return selected_cache_path_;
    }
    void set_selected_cache_path(const QString& path);

    int selected_cache_width() const {
        return selected_cache_width_;
    }
    int selected_cache_height() const {
        return selected_cache_height_;
    }
    int selected_cache_frame_count() const {
        return selected_cache_frame_count_;
    }
    QString selected_cache_format() const {
        return selected_cache_format_;
    }

    // -- Event Stats --
    QVariantList event_stats() const {
        return event_stats_;
    }

  signals:
    void statsChanged();
    void cacheListChanged();
    void selectedCachePathChanged();
    void internalScaleChanged();
    void wireframeChanged();

  private:
    static QString format_bytes(size_t bytes);
    void refresh_selected_cache_entry();

    // Performance
    float fps_ = 0.0f;
    float game_tick_ms_ = 0.0f;
    float tick_rate_hz_ = 0.0f;
    std::chrono::steady_clock::time_point last_drain_;
    int frame_count_ = 0;
    float fps_accum_ = 0.0f;

    // Tick section rolling averages.  Keyed by the const char* from
    // ProfileEntry::name — these are stable string literals owned by the
    // presenter, matching the SDL widget's approach.
    static constexpr int AVG_WINDOW = 30;
    struct RingBuffer {
        std::array<float, AVG_WINDOW> samples{};
        int write_pos = 0;
        bool filled = false;

        void push(float v) {
            samples[write_pos] = v;
            write_pos = (write_pos + 1) % AVG_WINDOW;
            if (write_pos == 0)
                filled = true;
        }
        float average() const {
            int count = filled ? AVG_WINDOW : write_pos;
            if (count == 0)
                return 0;
            float sum = 0;
            for (int i = 0; i < count; i++)
                sum += samples[i];
            return sum / count;
        }
    };
    std::unordered_map<const char*, RingBuffer> tick_avg_;
    QVariantList tick_sections_;

    // Parallel averaging state for Qt render-side phases.
    std::unordered_map<const char*, RingBuffer> qt_avg_;
    QVariantList qt_sections_;

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

    // Cache summary
    int cache_entries_ = 0;
    QString cache_used_;
    QString cache_max_;

    // Cache list (sampled every ~2s)
    QVariantList cache_list_;
    std::chrono::steady_clock::time_point last_cache_snapshot_;

    // Selected cache entry (for preview panel)
    QString selected_cache_path_;
    int selected_cache_width_ = 0;
    int selected_cache_height_ = 0;
    int selected_cache_frame_count_ = 0;
    QString selected_cache_format_;

    // Event Stats
    QVariantList event_stats_;
    std::chrono::steady_clock::time_point last_event_snapshot_;
};
