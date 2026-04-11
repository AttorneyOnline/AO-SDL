#include "CharIconProvider.h"

#include "QtImageWatcher.h"
#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"

#include <QQuickImageResponse>
#include <QQuickTextureFactory>

namespace {

/**
 * @brief Async response for a single character icon request.
 *
 * Lifetime is managed by Qt Quick.  The alive_ guard prevents the watcher
 * callback from touching this object after cancel() or destruction.
 */
class AsyncCharIconResponse : public QQuickImageResponse {
public:
    AsyncCharIconResponse(QtImageWatcher& watcher, const QString& id, const QSize& /*requestedSize*/) {
        const std::string path = "characters/" + id.toStdString() + "/char_icon";

        auto alive = m_alive;
        watcher.watch(path, [this, alive](std::shared_ptr<ImageAsset> asset) {
            if (!*alive)
                return;
            m_image = asset_frame_to_qimage(asset, kMaxThumb);
            emit finished();
        });
    }

    ~AsyncCharIconResponse() override { *m_alive = false; }

    // Called by Qt Quick when the requesting Image item is destroyed or its
    // source changes.  Prevent the pending callback from touching this object.
    void cancel() override { *m_alive = false; }

    QQuickTextureFactory* textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    static constexpr int kMaxThumb = 128;

    QImage m_image;
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
};

} // namespace

CharIconProvider::CharIconProvider(AssetLibrary& lib, QtImageWatcher& watcher)
    : m_lib(lib), m_watcher(watcher) {}

QQuickImageResponse* CharIconProvider::requestImageResponse(const QString& id,
                                                            const QSize& requestedSize) {
    return new AsyncCharIconResponse(m_watcher, id, requestedSize);
}
