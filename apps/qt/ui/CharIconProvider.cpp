#include "CharIconProvider.h"

#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"

CharIconProvider::CharIconProvider(AssetLibrary& lib)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_lib(lib) {}

QImage CharIconProvider::requestImage(const QString& id, QSize* size,
                                      const QSize& requestedSize) {
    // Check provider cache first (fast path for scrolling).
    {
        QReadLocker locker(&m_lock);
        auto it = m_cache.find(id);
        if (it != m_cache.end()) {
            const QImage& cached = it.value();
            if (size)
                *size = cached.size();
            if (requestedSize.isValid() && !requestedSize.isNull())
                return cached.scaled(requestedSize, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
            return cached;
        }
    }

    // Cache miss — decode from AssetLibrary.
    std::string path = "characters/" + id.toStdString() + "/char_icon";
    auto        img  = m_lib.image(path);
    if (!img || img->frame_count() == 0)
        return {};

    const auto& f = img->frame(0);
    // Construct QImage from the RGBA pixel buffer.  copy() detaches from the
    // ImageAsset-owned memory so the QImage remains valid after the shared_ptr
    // is released.
    QImage qimg(img->frame_pixels(0), f.width, f.height,
                f.width * 4, QImage::Format_RGBA8888);

    QImage result = qimg.copy();
    result.mirror(false, true);

    // Pre-scale to thumbnail size before caching.  Character icons are
    // displayed at 96×96 or smaller — storing full-resolution images
    // wastes memory and makes the scaling cost per-request instead of
    // once.
    static constexpr int kMaxThumb = 128;
    if (result.width() > kMaxThumb || result.height() > kMaxThumb)
        result = result.scaled(kMaxThumb, kMaxThumb, Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);

    // Store in provider cache so future requests skip the decode.
    {
        QWriteLocker locker(&m_lock);
        m_cache.insert(id, result);
    }

    if (size)
        *size = result.size();

    if (requestedSize.isValid() && !requestedSize.isNull()
        && (requestedSize.width() < result.width()
            || requestedSize.height() < result.height()))
        return result.scaled(requestedSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    return result;
}
