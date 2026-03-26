#pragma once

#include <QtQuick/QQuickItem>
#include <QtQml/qqml.h>

/**
 * @brief QML item that drives the game renderer and composites its output.
 *
 * Each frame SceneTextureItem::updatePaintNode() calls RenderBridge::renderFrame()
 * to advance the game scene, then wraps the resulting native texture in a
 * QSGSimpleTextureNode for Qt's scene graph to composite into the QML tree.
 *
 * UV orientation is handled automatically: when the backend reports
 * uv_flipped() (OpenGL), the texture node is mirrored vertically.
 *
 * Declare in QML as:
 * @code
 *   import AO 1.0
 *   SceneTextureItem { anchors.fill: parent }
 * @endcode
 */
class SceneTextureItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT

  public:
    explicit SceneTextureItem(QQuickItem* parent = nullptr);

  protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;
};
