import QtQuick

/**
 * Generic floating overlay container that adapts to its parent's bounds.
 *
 * Usage:
 *
 *   OverlayPanel {
 *       anchors { left: parent.left; top: parent.top; margins: 4 }
 *       backgroundColor: "#aa000000"
 *       backgroundRadius: 4
 *       padding: 6
 *
 *       Label { text: "Hello" }
 *   }
 *
 * By default the panel sizes itself to fit its content (via implicitWidth /
 * implicitHeight).  Set explicit width/height to override.
 *
 * The optional background rectangle is shown whenever backgroundColor has
 * non-zero alpha.  Set it to "transparent" (the default) for no background.
 */
Item {
    id: root

    // ── Public API ─────────────────────────────────────────────────────
    /** Items placed inside the OverlayPanel land here. */
    default property alias content: contentArea.data

    /** Background colour — set to a semi-transparent value for a HUD look.
     *  Default "transparent" means no background is rendered. */
    property color backgroundColor: "transparent"

    /** Corner radius for the background rectangle. */
    property real backgroundRadius: 0

    /** Inner padding between the background edge and the content. */
    property real padding: 0

    // ── Auto-sizing ────────────────────────────────────────────────────
    implicitWidth:  contentArea.childrenRect.width  + 2 * padding
    implicitHeight: contentArea.childrenRect.height + 2 * padding

    // ── Background ─────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color:   root.backgroundColor
        radius:  root.backgroundRadius
        visible: root.backgroundColor.a > 0
    }

    // ── Content area ───────────────────────────────────────────────────
    Item {
        id: contentArea
        anchors.fill:    parent
        anchors.margins: root.padding
    }
}
