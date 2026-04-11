#include "EmoteIconProvider.h"

#include "QtImageWatcher.h"
#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"

#include <QQuickImageResponse>
#include <QQuickTextureFactory>

#include <format>
#include <memory>

namespace {

/**
 * @brief Async response for a single emote icon request.
 *
 * Lifetime is managed by Qt Quick.  The alive_ guard prevents the watcher
 * callback from touching this object after cancel() or destruction.
 */
class AsyncEmoteIconResponse : public QQuickImageResponse {
public:
    AsyncEmoteIconResponse(QtImageWatcher& watcher, const QString& id,
                           const QSize& /*requestedSize*/) {
        // Parse "CharacterFolder/emoteIndex"
        const int slash = id.lastIndexOf('/');
        if (slash < 0) {
            // Malformed id — emit finished with empty image after construction.
            QMetaObject::invokeMethod(this, &QQuickImageResponse::finished, Qt::QueuedConnection);
            return;
        }
        const std::string character = id.left(slash).toStdString();
        bool ok = false;
        const int index = id.mid(slash + 1).toInt(&ok);
        if (!ok || index < 0) {
            QMetaObject::invokeMethod(this, &QQuickImageResponse::finished, Qt::QueuedConnection);
            return;
        }

        // AO2 convention: emotions/button{N}_off (matches AOAssetLibrary::emote_icon()).
        const std::string path =
            std::format("characters/{}/emotions/button{}_off", character, index + 1);

        auto alive = m_alive;
        watcher.watch(path, [this, alive](std::shared_ptr<ImageAsset> asset) {
            if (!*alive)
                return;
            m_image = asset_frame_to_qimage(asset, kMaxThumb);
            emit finished();
        });
    }

    ~AsyncEmoteIconResponse() override { *m_alive = false; }

    void cancel() override { *m_alive = false; }

    QQuickTextureFactory* textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    static constexpr int kMaxThumb = 64;

    QImage m_image;
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
};

} // namespace

EmoteIconProvider::EmoteIconProvider(AssetLibrary& lib, QtImageWatcher& watcher)
    : m_lib(lib), m_watcher(watcher) {}

QQuickImageResponse* EmoteIconProvider::requestImageResponse(const QString& id,
                                                             const QSize& requestedSize) {
    return new AsyncEmoteIconResponse(m_watcher, id, requestedSize);
}
