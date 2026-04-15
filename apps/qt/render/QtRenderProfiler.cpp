#include "QtRenderProfiler.h"

#include <QQuickWindow>

void QtRenderProfiler::install(QQuickWindow* window) {
    if (installed_ || !window)
        return;
    installed_ = true;

    // Signals fire on the render thread.  Qt::DirectConnection is required so
    // the handler runs synchronously in the signaling thread — a queued
    // connection would hop to the GUI thread and destroy the timing.
    connect(window, &QQuickWindow::beforeSynchronizing, this, [this] {
        // End of gui_gap: last afterFrameEnd → now we're entering sync.
        if (gui_gap_timer_.isValid())
            gui_gap_us_.store(static_cast<int>(gui_gap_timer_.nsecsElapsed() / 1000), std::memory_order_relaxed);
        sync_timer_.start();
    }, Qt::DirectConnection);

    connect(window, &QQuickWindow::afterSynchronizing, this, [this] {
        if (sync_timer_.isValid())
            sync_us_.store(static_cast<int>(sync_timer_.nsecsElapsed() / 1000), std::memory_order_relaxed);
        render_timer_.start();
    }, Qt::DirectConnection);

    connect(window, &QQuickWindow::afterRenderPassRecording, this, [this] {
        if (render_timer_.isValid())
            render_us_.store(static_cast<int>(render_timer_.nsecsElapsed() / 1000), std::memory_order_relaxed);
        present_timer_.start();
    }, Qt::DirectConnection);

    connect(window, &QQuickWindow::afterFrameEnd, this, [this] {
        if (present_timer_.isValid())
            present_us_.store(static_cast<int>(present_timer_.nsecsElapsed() / 1000), std::memory_order_relaxed);
        gui_gap_timer_.start();
    }, Qt::DirectConnection);
}
