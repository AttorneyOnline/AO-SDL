#pragma once

#include <QQuickImageProvider>

class AssetLibrary;

/**
 * @brief Synchronous QML image provider for the debug overlay's cache preview.
 *
 * Registered as "cachepreview" — QML accesses entries via:
 *   Image { source: "image://cachepreview/<virtual/path>" }
 *
 * Looks up the requested path via AssetLibrary::cache().peek() (non-promoting,
 * so the preview doesn't perturb LRU ordering).  Returns an empty QImage for
 * paths that are not currently cached or are not images.
 */
class CachePreviewProvider : public QQuickImageProvider {
public:
    explicit CachePreviewProvider(AssetLibrary& lib);

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    AssetLibrary& m_lib;
};
