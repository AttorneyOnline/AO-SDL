#pragma once

#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"

#include <QImage>
#include <QMutex>
#include <QObject>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Main-thread dispatcher for async AssetLibrary image availability.
 *
 * Runs every drain tick regardless of download activity.  For each watched
 * path it tries to decode via AssetLibrary::image(); on success the pending
 * callbacks are fired.  If decoding still fails, prefetch_image() is called
 * to re-queue the download — this transparently retries transient failures
 * (H2 stream-limit errors, connection resets) that clear MountHttp::pending_
 * without bumping cache_generation_.
 *
 * Thread contract:
 *   - watch()  may be called from any thread (provider worker or main).
 *   - drain()  must be called on the main thread, after HttpPool::poll().
 *
 * For immediate cache hits, watch() posts the callback to the main thread
 * via QMetaObject::invokeMethod so that it fires after requestImageResponse()
 * has returned — satisfying Qt Quick's requirement that finished() is not
 * emitted before the response object is returned to the engine.
 */
class QtImageWatcher : public QObject {
    Q_OBJECT
public:
    using ReadyCallback = std::function<void(std::shared_ptr<ImageAsset>)>;

    explicit QtImageWatcher(AssetLibrary& lib, QObject* parent = nullptr);

    /**
     * @brief Request notification when an image asset becomes available.
     *
     * @param path  Virtual path without extension (same key AssetLibrary::image() uses).
     * @param cb    Invoked on the main thread with the decoded ImageAsset once ready.
     *
     * If the asset is already cached, @p cb is posted to the main-thread event
     * loop (Qt::QueuedConnection on this object's thread) so it fires after
     * requestImageResponse() returns.  Otherwise it is stored and fired the next
     * time drain() succeeds in decoding the asset.
     *
     * Thread-safe.
     */
    void watch(const std::string& path, ReadyCallback cb);

    /**
     * @brief Attempt to resolve pending watchers.  Call once per main-thread tick,
     *        after HttpPool::poll() has delivered any new downloads.
     */
    void drain();

private:
    AssetLibrary& m_lib;

    mutable QMutex m_mutex;
    std::unordered_map<std::string, std::vector<ReadyCallback>> m_watchers;
};

/**
 * @brief Convert the first frame of an ImageAsset to a QImage suitable for QML display.
 *
 * Copies and vertically flips the pixel data to correct for the OpenGL bottom-up
 * texture convention used by the asset decoder.  Scales down to @p maxThumb if
 * either dimension exceeds it (preserving aspect ratio).  Pass 0 to skip scaling.
 */
QImage asset_frame_to_qimage(const std::shared_ptr<ImageAsset>& asset, int maxThumb = 0);
