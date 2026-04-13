import QtQuick

/**
 * Lightweight floating overlay container that sizes itself to its content.
 *
 * OverlayPanel is a thin wrapper that adds an optional semi-transparent
 * background rectangle and uniform padding around any child items.  It is
 * intended for HUD elements, tooltips, status badges, and any floating
 * UI that sits on top of the main viewport.
 *
 * By default the panel sizes itself to fit its content via implicitWidth /
 * implicitHeight.  Set explicit width / height to override auto-sizing.
 *
 * The background rectangle is only rendered when `backgroundColor` has a
 * non-zero alpha — leaving it at the default `"transparent"` costs nothing.
 *
 * ── Basic HUD badge ──────────────────────────────────────────────────────
 *
 *   OverlayPanel {
 *       anchors { top: parent.top; left: parent.left; margins: 4 }
 *       backgroundColor: "#aa000000"    // 67 % opaque black
 *       backgroundRadius: 4
 *       padding: 6
 *
 *       Label { text: "HP: 85 / 100"; color: "white"; font.pixelSize: 11 }
 *   }
 *
 * ── Multi-item status panel ──────────────────────────────────────────────
 *
 *   OverlayPanel {
 *       anchors { bottom: parent.bottom; right: parent.right; margins: 8 }
 *       backgroundColor: "#cc1a1a2e"
 *       backgroundRadius: 6
 *       padding: 8
 *
 *       ColumnLayout {
 *           spacing: 4
 *           Label { text: "Ping: 42 ms";   color: "#aaffaa"; font.pixelSize: 10 }
 *           Label { text: "Players: 12";   color: "#cccccc";  font.pixelSize: 10 }
 *           Label { text: "Server: Lobby"; color: "#cccccc";  font.pixelSize: 10 }
 *       }
 *   }
 *
 * ── Fixed-size overlay (disables auto-sizing) ────────────────────────────
 *
 *   OverlayPanel {
 *       width: 240; height: 120
 *       anchors.centerIn: parent
 *       backgroundColor: "#dd000000"
 *       backgroundRadius: 8
 *       padding: 12
 *
 *       Label {
 *           anchors.fill: parent
 *           wrapMode: Text.Wrap
 *           text: "Connection lost. Retrying…"
 *           color: "white"
 *           horizontalAlignment: Text.AlignHCenter
 *           verticalAlignment:   Text.AlignVCenter
 *       }
 *   }
 *
 * ── No background (transparent container) ───────────────────────────────
 *
 *   // backgroundColor defaults to "transparent" — no rect is drawn at all.
 *   OverlayPanel {
 *       anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
 *       padding: 4
 *
 *       Label { text: "Press F1 for help"; color: "#888888"; font.pixelSize: 10 }
 *   }
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
