#pragma once

#include <QImage>
#include <QQuickImageProvider>

class CharListModel;

/// Image provider serving character icons from the CharListModel's cached QImages.
///
/// Registered as "charicon" — QML accesses icons via:
///   Image { source: "image://charicon/<index>" }
class CharIconProvider : public QQuickImageProvider {
  public:
    explicit CharIconProvider(CharListModel& model);

    QImage requestImage(const QString& id, QSize* size, const QSize& requested_size) override;

  private:
    CharListModel& model_;
};
