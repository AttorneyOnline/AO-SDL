#include "QtImageWatcher.h"

#include <QMetaObject>
#include <QMutexLocker>

QtImageWatcher::QtImageWatcher(AssetLibrary& lib, QObject* parent)
    : QObject(parent), m_lib(lib) {}

void QtImageWatcher::watch(const std::string& path, ReadyCallback cb) {
    // Step 1: check AssetCache (decoded, GPU-ready pixels).
    // get_cached() is thread-safe (AssetCache has its own mutex) and promotes LRU.
    auto asset = std::dynamic_pointer_cast<ImageAsset>(m_lib.get_cached(path));

    if (!asset) {
        // Step 2: attempt a full decode via image() — handles the case where
        // the download completed before this watch() call.
        asset = m_lib.image(path);
    }

    if (!asset) {
        // Bytes are not available yet — queue the download.
        m_lib.prefetch_image(path);

        QMutexLocker locker(&m_mutex);
        // Final check under the lock: the download might have completed between
        // the image() call above and now, with drain() consuming the generation
        // bump before this callback was registered.  image() here decodes any
        // raw bytes that arrived in that window.
        asset = std::dynamic_pointer_cast<ImageAsset>(m_lib.get_cached(path));
        if (!asset)
            asset = m_lib.image(path);
        if (!asset) {
            m_watchers[path].push_back(std::move(cb));
            return;
        }
    }

    // Asset available — post to the main-thread event loop so the callback fires
    // after requestImageResponse() has returned to Qt Quick.  This satisfies
    // Qt Quick's requirement that finished() is not emitted before the response
    // object is handed back to the engine.
    QMetaObject::invokeMethod(
        this,
        [cb = std::move(cb), asset = std::move(asset)]() mutable { cb(std::move(asset)); },
        Qt::QueuedConnection);
}

void QtImageWatcher::drain() {
    // Run every tick — no generation gate.  Reasons:
    //
    // 1. Transient download failures (H2 stream-limit errors, connection resets)
    //    clear MountHttp::pending_ without bumping cache_generation_, so a
    //    generation-gated drain would never see them.  The prefetch_image()
    //    call below retries those downloads transparently.
    //
    // 2. The cost per tick is small: one hash lookup per watcher per extension
    //    (O(1)), and prefetch_image() is idempotent (no-op if already pending
    //    or 404-failed).
    QMutexLocker locker(&m_mutex);
    if (m_watchers.empty())
        return;

    // Collect resolved entries before releasing the lock so callbacks cannot
    // re-enter the watcher and deadlock.
    using Entry = std::pair<std::vector<ReadyCallback>, std::shared_ptr<ImageAsset>>;
    std::vector<Entry> to_fire;
    std::vector<std::string> to_retry;

    for (auto it = m_watchers.begin(); it != m_watchers.end();) {
        auto asset = std::dynamic_pointer_cast<ImageAsset>(m_lib.get_cached(it->first));
        if (!asset)
            asset = m_lib.image(it->first);

        if (asset) {
            to_fire.emplace_back(std::move(it->second), std::move(asset));
            it = m_watchers.erase(it);
        } else {
            to_retry.push_back(it->first);
            ++it;
        }
    }

    locker.unlock();

    // Re-queue downloads for paths whose first attempt transiently failed.
    // MountHttp::request() deduplicates: no-op if already pending or 404.
    for (const auto& path : to_retry)
        m_lib.prefetch_image(path);

    // Fire callbacks on the main thread (drain() is always called from there).
    for (auto& [callbacks, asset] : to_fire)
        for (auto& cb : callbacks)
            cb(asset);
}

QImage asset_frame_to_qimage(const std::shared_ptr<ImageAsset>& asset, int maxThumb) {
    if (!asset || asset->frame_count() == 0)
        return {};

    const auto& f = asset->frame(0);
    // Build a non-owning view, then immediately copy to get independent storage
    // before flipping.  The copy detaches from ImageAsset's AlignedBuffer so the
    // QImage remains valid after the shared_ptr is released.
    QImage qimg(asset->frame_pixels(0), f.width, f.height, f.width * 4, QImage::Format_RGBA8888);
    QImage result = qimg.copy();
    result.flip(Qt::Vertical);

    if (maxThumb > 0 && (result.width() > maxThumb || result.height() > maxThumb))
        result = result.scaled(maxThumb, maxThumb, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return result;
}
