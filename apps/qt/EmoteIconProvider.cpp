#include "EmoteIconProvider.h"

#include "QmlUIBridge.h"

EmoteIconProvider::EmoteIconProvider(QmlUIBridge& bridge)
    : QQuickImageProvider(QQuickImageProvider::Image), bridge_(bridge) {
}

QImage EmoteIconProvider::requestImage(const QString& id, QSize* size, const QSize& /*requested_size*/) {
    // Strip query string (e.g. "5?g=3" → "5")
    int index = id.section('?', 0, 0).toInt();

    QImage img = bridge_.emote_icon_image(index);
    if (img.isNull())
        return {};

    if (size)
        *size = img.size();

    return img;
}
