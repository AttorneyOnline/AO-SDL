#pragma once

#include <QQuickAsyncImageProvider>

class AssetLibrary;
class QtImageWatcher;

/**
 * @brief Async QML image provider for character icons.
 *
 * Registered as "charicon" — QML accesses icons via:
 *   Image { source: "image://charicon/<CharacterFolder>" }
 *
 * Returns a QQuickImageResponse immediately.  The QML Image element shows
 * a loading placeholder until the asset is decoded and the response emits
 * finished().
 *
 * No internal QImage cache is maintained here.  Decoded pixels live in
 * AssetLibrary's LRU cache; Qt's QSG texture cache handles GPU-side
 * deduplication.
 */
class CharIconProvider : public QQuickAsyncImageProvider {
public:
    explicit CharIconProvider(AssetLibrary& lib, QtImageWatcher& watcher);

    QQuickImageResponse* requestImageResponse(const QString& id,
                                              const QSize& requestedSize) override;

private:
    AssetLibrary& m_lib;
    QtImageWatcher& m_watcher;
};
