#include "CharIconProvider.h"

#include "QmlUIBridge.h"

CharIconProvider::CharIconProvider(CharListModel& model)
    : QQuickImageProvider(QQuickImageProvider::Image), model_(model) {
}

QImage CharIconProvider::requestImage(const QString& id, QSize* size, const QSize& /*requested_size*/) {
    // Strip query string (e.g. "81?g=86" → "81")
    int index = id.section('?', 0, 0).toInt();

    QImage img = model_.icon_image(index);
    if (img.isNull())
        return {};

    if (size)
        *size = img.size();

    return img;
}
