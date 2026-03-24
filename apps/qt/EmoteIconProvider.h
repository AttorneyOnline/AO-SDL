#pragma once

#include <QImage>
#include <QQuickImageProvider>

class QmlUIBridge;

/// Image provider serving emote button icons from the QmlUIBridge's cached QImages.
///
/// Registered as "emoteicon" — QML accesses icons via:
///   Image { source: "image://emoteicon/<index>" }
class EmoteIconProvider : public QQuickImageProvider {
  public:
    explicit EmoteIconProvider(QmlUIBridge& bridge);

    QImage requestImage(const QString& id, QSize* size, const QSize& requested_size) override;

  private:
    QmlUIBridge& bridge_;
};
