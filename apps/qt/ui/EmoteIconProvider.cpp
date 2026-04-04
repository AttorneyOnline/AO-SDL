#include "EmoteIconProvider.h"

#include "ao/asset/AOAssetLibrary.h"
#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"

EmoteIconProvider::EmoteIconProvider(AssetLibrary& lib) : QQuickImageProvider(QQuickImageProvider::Image), m_lib(lib) {
}

QImage EmoteIconProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    // Fast path: already decoded and cached.
    {
        QReadLocker locker(&m_lock);
        auto it = m_cache.find(id);
        if (it != m_cache.end()) {
            const QImage& cached = it.value();
            if (size)
                *size = cached.size();
            if (requestedSize.isValid() && !requestedSize.isNull())
                return cached.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return cached;
        }
    }

    // Parse "CharacterFolder/emoteIndex".
    int slash = id.lastIndexOf('/');
    if (slash < 0)
        return {};
    const QString character = id.left(slash);
    bool ok = false;
    const int index = id.mid(slash + 1).toInt(&ok);
    if (!ok || index < 0)
        return {};

    // Resolve via AO asset convention (emotions/button{N}_off).
    AOAssetLibrary ao(m_lib);
    auto img = ao.emote_icon(character.toStdString(), index);
    if (!img || img->frame_count() == 0)
        return {};

    const auto& f = img->frame(0);
    QImage qimg(img->frame_pixels(0), f.width, f.height, f.width * 4, QImage::Format_RGBA8888);
    QImage result = qimg.copy();
    result.flip(Qt::Vertical);

    // Pre-scale to button size before caching — emote icons are small (≤64px).
    static constexpr int kMaxThumb = 64;
    if (result.width() > kMaxThumb || result.height() > kMaxThumb)
        result = result.scaled(kMaxThumb, kMaxThumb, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    {
        QWriteLocker locker(&m_lock);
        m_cache.insert(id, result);
    }

    if (size)
        *size = result.size();
    if (requestedSize.isValid() && !requestedSize.isNull())
        return result.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return result;
}
