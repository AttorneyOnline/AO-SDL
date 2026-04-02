#pragma once

#include <QHash>
#include <QImage>
#include <QQuickImageProvider>
#include <QReadWriteLock>

class AssetLibrary;

/**
 * @brief QML image provider for emote button icons.
 *
 * Registered as "emoteicon" — QML accesses icons via:
 *   Image { source: "image://emoteicon/<CharacterFolder>/<emoteIndex>" }
 *
 * Delegates to AOAssetLibrary::emote_icon() (emotions/button{N}_off path
 * convention) and caches the decoded QImage so repeated scrolling through
 * the emote grid is a cheap hash lookup.  Thread-safe: requestImage() may
 * be called from a QML worker thread when Image.asynchronous is true.
 */
class EmoteIconProvider : public QQuickImageProvider {
  public:
    explicit EmoteIconProvider(AssetLibrary& lib);

    QImage requestImage(const QString& id, QSize* size,
                        const QSize& requestedSize) override;

  private:
    AssetLibrary&          m_lib;
    QHash<QString, QImage> m_cache;
    mutable QReadWriteLock m_lock;
};
