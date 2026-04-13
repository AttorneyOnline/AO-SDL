import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * A single collapsible section for use inside an AccordionDrawer.
 *
 * Each section has a clickable header bar and an animated content area.
 * The section does not manage exclusivity on its own — that is handled
 * by the parent AccordionDrawer.
 *
 *   AccordionDrawer {
 *       AccordionSection {
 *           title: "General"
 *           Label { text: "General settings here" }
 *       }
 *       AccordionSection {
 *           title: "Advanced"
 *           CheckBox { text: "Enable feature X" }
 *       }
 *   }
 */
Item {
    id: root

    // ── Public API ─────────────────────────────────────────────────────

    /** Header text shown in the section bar. */
    property string title: ""

    /** Whether the content area is currently shown. */
    property bool expanded: false

    /** Animation duration in milliseconds.  Set to 0 to disable. */
    property int animationDuration: 200

    /** Items placed inside the AccordionSection land in the content area. */
    default property alias content: contentArea.data

    /** Emitted when the user clicks the header to toggle this section. */
    signal toggled()

    // ── Internals ──────────────────────────────────────────────────────
    readonly property real _headerHeight: 28
    readonly property real _contentHeight: contentArea.childrenRect.height
    readonly property real _targetHeight: _headerHeight + (expanded ? _contentHeight : 0)

    property real _animH: _headerHeight

    Behavior on _animH {
        NumberAnimation { duration: root.animationDuration; easing.type: Easing.OutCubic }
    }
    on_TargetHeightChanged: _animH = _targetHeight

    implicitHeight: _animH
    implicitWidth: parent ? parent.width : 200

    clip: true

    // ── Header bar ─────────────────────────────────────────────────────
    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: root._headerHeight
        color: headerMouse.containsMouse ? "#363636" : "#2a2a2a"

        RowLayout {
            anchors { fill: parent; leftMargin: 6; rightMargin: 6 }
            spacing: 4

            Label {
                text: root.expanded ? "\u25BE" : "\u25B8"
                font.pixelSize: 10
                color: "#999999"
            }

            Label {
                text: root.title
                font.bold: true
                font.pixelSize: 11
                color: "#cccccc"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        MouseArea {
            id: headerMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.toggled()
        }
    }

    // ── Content area ───────────────────────────────────────────────────
    Item {
        id: contentArea
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
        }
        height: root._contentHeight
    }
}
