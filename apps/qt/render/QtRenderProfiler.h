#pragma once

#include <QElapsedTimer>
#include <QObject>

#include <atomic>

class QQuickWindow;

/**
 * @brief Samples Qt Scene Graph per-frame phase timings for the debug overlay.
 *
 * Installs Qt::DirectConnection handlers on a QQuickWindow's lifecycle signals,
 * so each measurement runs synchronously on the render thread (for sync/render/
 * present) or the GUI thread (for the frame-gap delta).  The resulting
 * microsecond durations are published as lock-free atomics that
 * DebugController polls on its drain() tick.
 *
 * Phases (mirrors how the SDL frontend times the engine presenter tick):
 *   - sync:       beforeSynchronizing → afterSynchronizing
 *                 (scene-graph snapshot from GUI thread items — includes QML
 *                  binding updates that were deferred to sync).
 *   - render:     beforeRendering → afterRenderPassRecording
 *                 (QSG render-pass command encoding; dominated by SceneTextureItem
 *                  and any Canvas/Text items).
 *   - present:    afterRenderPassRecording → afterFrameEnd
 *                 (GPU submit + swap / vsync wait).
 *   - gui_gap:    afterFrameEnd → next beforeSynchronizing
 *                 (GUI-thread time between frames: event loop, bindings, layout).
 *
 * Sum of these equals one full QSG frame; plotting them in a pie chart mirrors
 * the engine tick-breakdown visualization and surfaces interop hot spots.
 */
class QtRenderProfiler : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QtRenderProfiler)

  public:
    struct Section {
        const char* name;
        const std::atomic<int>* us;
    };

    static QtRenderProfiler& instance() {
        static QtRenderProfiler p;
        return p;
    }

    /// Install signal handlers on the given window.  May be called exactly
    /// once; subsequent calls are ignored.  The window must outlive the
    /// QGuiApplication (the profiler is a static singleton).
    void install(QQuickWindow* window);

    /// Snapshot the currently installed sections (name + atomic pointer).
    /// Safe to call from any thread.
    std::vector<Section> sections() const {
        return {
            {"gui_gap", &gui_gap_us_},
            {"sync", &sync_us_},
            {"render", &render_us_},
            {"present", &present_us_},
        };
    }

  private:
    QtRenderProfiler() = default;

    // All timers run on the render thread except frame_gap_timer_ which is
    // restarted on the GUI thread via afterFrameEnd (emitted on the render
    // thread, but we hop to the GUI thread via queued connection).
    QElapsedTimer sync_timer_;
    QElapsedTimer render_timer_;
    QElapsedTimer present_timer_;
    QElapsedTimer gui_gap_timer_;

    std::atomic<int> sync_us_{0};
    std::atomic<int> render_us_{0};
    std::atomic<int> present_us_{0};
    std::atomic<int> gui_gap_us_{0};

    bool installed_ = false;
};
