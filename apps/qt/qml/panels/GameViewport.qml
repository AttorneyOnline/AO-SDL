import QtQuick

/**
 * Game scene viewport — the only element that is simultaneously a Component
 * (backed by the C++ SceneTextureItem QML_ELEMENT) and a Panel (owned by
 * CourtroomScreen and tied to the render pipeline via RenderBridge).
 *
 * The outer Item fills whatever space the caller allocates.  The inner
 * SceneTextureItem scales to the largest rectangle that preserves the ratio
 * defined by baseWidth : baseHeight, then centres itself.
 *
 * Callers that need to overlay content exactly over the rendered scene (HUD
 * elements, interjection flash, IC log) should use the sceneX / sceneY /
 * sceneWidth / sceneHeight aliases rather than the outer item's bounds.
 *
 * To change the aspect ratio, edit baseWidth / baseHeight here — all callers
 * adapt automatically.  smooth: false preserves pixel-art fidelity.
 */
Item {
    id: root

    // AO base resolution — defines the display aspect ratio.
    readonly property int baseWidth:  256
    readonly property int baseHeight: 192

    // Exact bounds of the rendered rectangle within this item's coordinate
    // space.  Bind a sibling overlay Item to these to avoid covering the
    // surrounding black space.
    readonly property alias sceneX:      scene.x
    readonly property alias sceneY:      scene.y
    readonly property alias sceneWidth:  scene.width
    readonly property alias sceneHeight: scene.height

    SceneTextureItem {
        id: scene
        readonly property real scaleFactor: Math.min(root.width  / root.baseWidth,
                                                     root.height / root.baseHeight)
        width:  Math.round(root.baseWidth  * scaleFactor)
        height: Math.round(root.baseHeight * scaleFactor)
        anchors.centerIn: parent
        smooth: false
    }
}
