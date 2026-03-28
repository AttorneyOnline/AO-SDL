#pragma once

#include <QHash>
#include <QImage>
#include <QQuickImageProvider>
#include <QReadWriteLock>

class AssetLibrary;

/**
 * @brief QML image provider that serves character icons from AssetLibrary.
 *
 * Registered as "charicon" — QML accesses icons via:
 *   Image { source: "image://charicon/CharacterFolder" }
 *
 * Decoded QImages are cached internally so that scrolling through an
 * already-loaded grid is a cheap hash lookup rather than a full decode,
 * copy, and mirror per icon.  The cache is guarded by a read-write lock
 * because QML may call requestImage() from a worker thread when
 * Image.asynchronous is true.
 */
class CharIconProvider : public QQuickImageProvider {
  public:
    explicit CharIconProvider(AssetLibrary& lib);

    QImage requestImage(const QString& id, QSize* size,
                        const QSize& requestedSize) override;

  private:
    AssetLibrary&          m_lib;
    QHash<QString, QImage> m_cache;
    mutable QReadWriteLock m_lock;
};
