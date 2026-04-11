#pragma once

#include <QQuickAsyncImageProvider>

class AssetLibrary;
class QtImageWatcher;

/**
 * @brief Async QML image provider for emote button icons.
 *
 * Registered as "emoteicon" — QML accesses icons via:
 *   Image { source: "image://emoteicon/<CharacterFolder>/<emoteIndex>" }
 *
 * Delegates to the AO2 path convention for emote icons
 * (characters/<char>/emotions/button<N>_off).
 *
 * Returns a QQuickImageResponse immediately.  finished() is emitted once the
 * asset is decoded and available in AssetLibrary's LRU cache.  No internal
 * QImage cache is maintained — removing the duplicate pixel allocation that
 * the old synchronous provider required.
 */
class EmoteIconProvider : public QQuickAsyncImageProvider {
public:
    explicit EmoteIconProvider(AssetLibrary& lib, QtImageWatcher& watcher);

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize& requestedSize) override;

private:
    AssetLibrary& m_lib;
    QtImageWatcher& m_watcher;
};
