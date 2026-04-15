#include "CachePreviewProvider.h"

#include "QtImageWatcher.h" // asset_frame_to_qimage()
#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"

CachePreviewProvider::CachePreviewProvider(AssetLibrary& lib) : QQuickImageProvider(QQuickImageProvider::Image), m_lib(lib) {
}

QImage CachePreviewProvider::requestImage(const QString& id, QSize* size, const QSize& /*requestedSize*/) {
    auto asset = m_lib.cache().peek(id.toStdString());
    auto img = std::dynamic_pointer_cast<ImageAsset>(asset);
    if (!img)
        return {};

    QImage out = asset_frame_to_qimage(img, 0);
    if (size)
        *size = out.size();
    return out;
}
