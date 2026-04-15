#include "DebugController.h"

#include "QtDebugContext.h"
#include "render/QtRenderProfiler.h"
#include "asset/AssetCache.h"
#include "asset/ImageAsset.h"
#include "asset/MediaManager.h"
#include "asset/MountManager.h"
#include "event/EventManager.h"
#include "event/PlayerCountEvent.h"
#include "event/ServerInfoEvent.h"
#include "game/GameThread.h"
#include "game/IScenePresenter.h"

#include <QVariantMap>

#include <cmath>

DebugController::DebugController(QObject* parent)
    : QObject(parent), last_drain_(std::chrono::steady_clock::now()),
      last_cache_snapshot_(std::chrono::steady_clock::now() - std::chrono::seconds(10)),
      last_event_snapshot_(std::chrono::steady_clock::now()) {
}

DebugController::~DebugController() = default;

// --- Renderer (read/write through QtDebugContext atomics) --------------------

QString DebugController::backend_name() const {
    return QString::fromLatin1(QtDebugContext::instance().backend_name);
}

int DebugController::internal_scale() const {
    return QtDebugContext::instance().internal_scale.load(std::memory_order_relaxed);
}

void DebugController::set_internal_scale(int scale) {
    auto& ctx = QtDebugContext::instance();
    if (ctx.internal_scale.load(std::memory_order_relaxed) != scale) {
        ctx.internal_scale.store(scale, std::memory_order_relaxed);
        emit internalScaleChanged();
    }
}

bool DebugController::wireframe() const {
    return QtDebugContext::instance().wireframe.load(std::memory_order_relaxed);
}

void DebugController::set_wireframe(bool on) {
    auto& ctx = QtDebugContext::instance();
    if (ctx.wireframe.load(std::memory_order_relaxed) != on) {
        ctx.wireframe.store(on, std::memory_order_relaxed);
        emit wireframeChanged();
    }
}

// --- Drain ------------------------------------------------------------------

void DebugController::drain() {
    auto now = std::chrono::steady_clock::now();

    // -- FPS (smoothed over ~1 second) --
    auto delta = std::chrono::duration<float>(now - last_drain_).count();
    last_drain_ = now;

    if (delta > 0.0f) {
        float instant_fps = 1.0f / delta;
        constexpr float alpha = 0.05f;
        fps_ = (fps_ < 1.0f) ? instant_fps : fps_ + alpha * (instant_fps - fps_);
    }

    // -- Game thread stats --
    auto& ctx = QtDebugContext::instance();
    if (ctx.game_thread) {
        game_tick_ms_ = static_cast<float>(ctx.game_thread->last_tick_us()) / 1000.0f;
        tick_rate_hz_ = ctx.game_thread->tick_rate_hz();
    }

    // -- Tick breakdown (pie slices with rolling averages) --
    if (ctx.presenter) {
        auto profile = ctx.presenter->tick_profile();
        QVariantList slices;
        slices.reserve(static_cast<int>(profile.size()));
        for (const auto& entry : profile) {
            float us = static_cast<float>(entry.us->load(std::memory_order_relaxed));
            tick_avg_[entry.name].push(us);

            QVariantMap slice;
            slice[QStringLiteral("name")] = QString::fromLatin1(entry.name);
            slice[QStringLiteral("us")] = us;
            slice[QStringLiteral("avgUs")] = tick_avg_[entry.name].average();
            slices.append(slice);
        }
        tick_sections_ = std::move(slices);
        frame_count_++;
    }

    // -- Qt Scene Graph phase timings (sync/render/present/gui_gap) --
    {
        auto phases = QtRenderProfiler::instance().sections();
        QVariantList slices;
        slices.reserve(static_cast<int>(phases.size()));
        for (const auto& p : phases) {
            float us = static_cast<float>(p.us->load(std::memory_order_relaxed));
            qt_avg_[p.name].push(us);

            QVariantMap slice;
            slice[QStringLiteral("name")] = QString::fromLatin1(p.name);
            slice[QStringLiteral("us")] = us;
            slice[QStringLiteral("avgUs")] = qt_avg_[p.name].average();
            slices.append(slice);
        }
        qt_sections_ = std::move(slices);
    }

    // -- Connection events --
    auto& info_ch = EventManager::instance().get_channel<ServerInfoEvent>();
    if (auto ev = info_ch.get_event()) {
        server_info_ = QString::fromStdString(ev->get_software()) + " " + QString::fromStdString(ev->get_version());
        has_connection_ = true;
        conn_state_ = "Connected";
    }

    auto& count_ch = EventManager::instance().get_channel<PlayerCountEvent>();
    if (auto ev = count_ch.get_event()) {
        current_players_ = ev->get_current();
        max_players_ = ev->get_max();
    }

    // -- HTTP stats --
    auto http = MediaManager::instance().mounts_ref().http_stats();
    http_pending_ = http.pending;
    http_cached_ = http.cached;
    http_failed_ = http.failed;
    http_pool_pending_ = http.pool_pending;
    http_cached_size_ = format_bytes(http.cached_bytes);

    // -- Asset cache summary --
    const auto& cache = MediaManager::instance().assets().cache();
    cache_entries_ = static_cast<int>(cache.entry_count());
    cache_used_ = format_bytes(cache.used_bytes());
    cache_max_ = format_bytes(cache.max_bytes());

    // -- Asset cache list (resampled every ~2s to avoid UI flicker) --
    auto since_cache_snapshot = std::chrono::duration<float>(now - last_cache_snapshot_).count();
    if (since_cache_snapshot >= 2.0f) {
        last_cache_snapshot_ = now;
        auto snapshot = cache.snapshot_lru();
        QVariantList list;
        list.reserve(static_cast<int>(snapshot.size()));
        for (const auto& e : snapshot) {
            QVariantMap entry;
            entry[QStringLiteral("path")] = QString::fromStdString(e.path);
            entry[QStringLiteral("format")] = QString::fromStdString(e.format);
            entry[QStringLiteral("bytes")] = static_cast<qulonglong>(e.bytes);
            entry[QStringLiteral("bytesFmt")] = format_bytes(e.bytes);
            entry[QStringLiteral("useCount")] = static_cast<qlonglong>(e.use_count);
            entry[QStringLiteral("width")] = e.width;
            entry[QStringLiteral("height")] = e.height;
            entry[QStringLiteral("frameCount")] = e.frame_count;
            list.append(entry);
        }
        cache_list_ = std::move(list);
        emit cacheListChanged();

        // Refresh selected entry's metadata (dimensions change when
        // evicted/reloaded, though the path key stays the same).
        if (!selected_cache_path_.isEmpty())
            refresh_selected_cache_entry();
    }

    // -- Event stats (refresh every ~1 second to avoid overhead) --
    auto since_snapshot = std::chrono::duration<float>(now - last_event_snapshot_).count();
    if (since_snapshot >= 1.0f) {
        last_event_snapshot_ = now;
        auto stats = EventManager::instance().snapshot_channel_stats();
        QVariantList list;
        list.reserve(static_cast<int>(stats.size()));
        for (const auto& s : stats) {
            QVariantMap entry;
            entry[QStringLiteral("name")] = QString::fromLatin1(s.raw_name);
            entry[QStringLiteral("count")] = static_cast<qulonglong>(s.count);
            list.append(entry);
        }
        event_stats_ = std::move(list);
    }

    emit statsChanged();
}

void DebugController::set_selected_cache_path(const QString& path) {
    if (path == selected_cache_path_)
        return;
    selected_cache_path_ = path;
    refresh_selected_cache_entry();
    emit selectedCachePathChanged();
}

void DebugController::refresh_selected_cache_entry() {
    selected_cache_width_ = 0;
    selected_cache_height_ = 0;
    selected_cache_frame_count_ = 0;
    selected_cache_format_.clear();

    if (selected_cache_path_.isEmpty())
        return;

    auto asset = MediaManager::instance().assets().cache().peek(selected_cache_path_.toStdString());
    if (!asset)
        return;

    selected_cache_format_ = QString::fromStdString(asset->format());
    if (auto img = std::dynamic_pointer_cast<ImageAsset>(asset)) {
        selected_cache_width_ = img->width();
        selected_cache_height_ = img->height();
        selected_cache_frame_count_ = img->frame_count();
    }
}

// --- Helpers ----------------------------------------------------------------

QString DebugController::format_bytes(size_t bytes) {
    if (bytes < 1024)
        return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024)
        return QString::number(static_cast<double>(bytes) / 1024.0, 'f', 1) + " KB";
    return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f', 1) + " MB";
}
