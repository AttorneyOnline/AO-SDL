import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Wrapper that lets any panel be popped out into a draggable, resizable
 * floating tool window with a single click.
 *
 * While docked the panel renders inline inside its parent layout.  Clicking
 * the ⤢ button pops it out: the inline slot collapses to a slim 22 px re-dock
 * strip (so sibling panels reclaim the space) and the content moves into a
 * frameless tool window the user can drag and resize freely.  Clicking ↩ in
 * the strip or ✕ in the floating window re-docks the panel.
 *
 * DockPanel sets the correct SplitView.*, Layout.*, and implicitHeight
 * attached properties automatically, so it works in SplitView,
 * ColumnLayout, RowLayout, or a plain Item without any extra wiring.
 * Do NOT set SplitView.* or Layout.* height properties on this item
 * yourself — they are managed internally.
 *
 * ── Inside a SplitView ──────────────────────────────────────────────────
 *
 *   SplitView {
 *       orientation: Qt.Vertical
 *
 *       DockPanel {
 *           title:               "Music & Areas"
 *           dockedHeight:        200     // preferred height while docked
 *           dockedMinimumHeight: 80      // user can't drag below this
 *           undockedSize:        Qt.size(280, 400)
 *
 *           panelComponent: Component {
 *               MusicAreaPanel { controller: root.controller }
 *           }
 *       }
 *
 *       DockPanel {
 *           title:        "Chat"
 *           dockedHeight: 180
 *           panelComponent: Component { OOCChatPanel { } }
 *       }
 *   }
 *
 * ── Inside a ColumnLayout ────────────────────────────────────────────────
 *
 *   ColumnLayout {
 *       DockPanel {
 *           title:        "Player List"
 *           dockedHeight: 160
 *           panelComponent: Component { PlayerListPanel { } }
 *       }
 *   }
 *
 * ── Controlling the docked state from code ───────────────────────────────
 *
 *   DockPanel {
 *       id: evidencePanel
 *       title: "Evidence"
 *       …
 *   }
 *
 *   Button { text: "Pop out"; onClicked: evidencePanel.docked = false }
 *   Button { text: "Re-dock"; onClicked: evidencePanel.docked = true  }
 *
 * ── Overlay mode (panel lives outside a layout) ──────────────────────────
 *
 * Set `overlay: true` when the DockPanel is used as a centred overlay
 * rather than part of a SplitView or Layout.  This disables the
 * collapse-to-strip behaviour so the panel always shows at its full size.
 *
 *   DockPanel {
 *       title:        "Emote Selector"
 *       overlay:      true
 *       undockedSize: Qt.size(400, 320)
 *       width: 400; height: 320
 *       anchors.centerIn: parent
 *
 *       panelComponent: Component { EmoteSelectorPanel { } }
 *   }
 */
Item {
    id: root

    // ── Public API ─────────────────────────────────────────────────────
    /** Panel title shown in the title bar (docked and floating). */
    property string title: ""

    /** If true, the panel renders inline; if false, it floats in a tool window. */
    property bool docked: true

    /** Set to true when the DockPanel lives outside a SplitView (e.g. as a
     *  centered overlay).  Disables the SplitView collapse/expand behaviour. */
    property bool overlay: false

    /** Preferred height when docked inside a SplitView. */
    property real dockedHeight: 200

    /** Minimum height the user can resize to while docked. */
    property real dockedMinimumHeight: 60

    /** Size of the floating tool window when undocked. */
    property size undockedSize: Qt.size(300, 240)

    /** The component to instantiate as the panel content. */
    property Component panelComponent: null

    // ── Layout integration (internal) ────────────────────────────────
    // Attached properties for every supported parent container type.
    // QML silently ignores attached properties that don't match the
    // parent, so setting both SplitView.* and Layout.* is safe and lets
    // DockPanel work in a SplitView, ColumnLayout, or plain Item without
    // the caller needing to know or care.

    readonly property real _collapsedHeight: 22
    readonly property real _currentHeight:   overlay ? dockedHeight
                                           : (docked ? dockedHeight : _collapsedHeight)

    // Universal — any parent that reads implicit size
    implicitHeight: _currentHeight

    // SplitView / Layout parent.  -1 is the "no preference" sentinel both
    // SplitView and QtQuick.Layouts use — assigning `undefined` to these
    // double-typed attached properties is rejected by the QML engine and
    // leaves stale values behind.
    SplitView.preferredHeight: overlay ? -1 : _currentHeight
    SplitView.minimumHeight:   overlay ? -1
                             : (docked ? dockedMinimumHeight : _collapsedHeight)
    SplitView.maximumHeight:   overlay ? -1
                             : (docked ? Number.POSITIVE_INFINITY : _collapsedHeight)

    Layout.preferredHeight: overlay ? -1 : _currentHeight
    Layout.minimumHeight:   overlay ? -1
                          : (docked ? dockedMinimumHeight : _collapsedHeight)
    Layout.maximumHeight:   overlay ? -1
                          : (docked ? Number.POSITIVE_INFINITY : _collapsedHeight)
    Layout.fillWidth: !overlay

    // ── Docked view ────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        visible: root.docked
        spacing: 0

        // Title / toolbar strip
        Rectangle {
            Layout.fillWidth: true
            height: 22
            color: "#2a2a2a"

            RowLayout {
                anchors { fill: parent; leftMargin: 6; rightMargin: 2 }
                spacing: 4

                Label {
                    text: root.title
                    font.bold: true
                    font.pixelSize: 10
                    color: "#cccccc"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                ToolButton {
                    text: "\u2922"
                    font.pixelSize: 11
                    implicitWidth:  20
                    implicitHeight: 20
                    ToolTip.text:    "Undock"
                    ToolTip.visible: hovered
                    onClicked: root.docked = false
                }
            }
        }

        // Inline content — active only while docked
        Loader {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            active:            root.docked
            sourceComponent:   root.panelComponent
        }
    }

    // ── Undocked placeholder (compact strip so the layout reclaims space) ──
    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: root._collapsedHeight
        visible: !root.docked && !root.overlay
        color: "#2a2a2a"

        RowLayout {
            anchors { fill: parent; leftMargin: 6; rightMargin: 2 }
            spacing: 4

            Label {
                text: root.title
                font.bold: true
                font.pixelSize: 10
                color: "#666666"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            ToolButton {
                text: "\u21A9"
                font.pixelSize: 11
                implicitWidth:  20
                implicitHeight: 20
                ToolTip.text:    "Re-dock"
                ToolTip.visible: hovered
                onClicked: root.docked = true
            }
        }
    }

    // ── Floating window ────────────────────────────────────────────────
    Window {
        id: floatWin
        visible: !root.docked && root.visible
        width:   root.undockedSize.width
        height:  root.undockedSize.height
        minimumWidth:  150
        minimumHeight: 80
        flags:   Qt.Tool | Qt.FramelessWindowHint

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Custom title bar — draggable
            Rectangle {
                Layout.fillWidth: true
                height: 24
                color: "#2a2a2a"

                DragHandler {
                    target: null
                    onActiveChanged: if (active) floatWin.startSystemMove()
                }

                RowLayout {
                    anchors { fill: parent; leftMargin: 6; rightMargin: 2 }
                    spacing: 4

                    Label {
                        text: root.title
                        font.bold: true
                        font.pixelSize: 10
                        color: "#cccccc"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    ToolButton {
                        text: "\u2715"
                        font.pixelSize: 11
                        implicitWidth:  20
                        implicitHeight: 20
                        ToolTip.text:    "Close"
                        ToolTip.visible: hovered
                        onClicked: root.docked = true
                    }
                }
            }

            // Panel content — active only while undocked
            Loader {
                Layout.fillWidth:  true
                Layout.fillHeight: true
                active:          !root.docked
                sourceComponent: root.panelComponent
            }
        }

        // ── Resize handles ─────────────────────────────────────────────
        // Edges
        MouseArea {
            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
            width: 4; cursorShape: Qt.SizeHorCursor
            onPressed: floatWin.startSystemResize(Qt.LeftEdge)
        }
        MouseArea {
            anchors { right: parent.right; top: parent.top; bottom: parent.bottom }
            width: 4; cursorShape: Qt.SizeHorCursor
            onPressed: floatWin.startSystemResize(Qt.RightEdge)
        }
        MouseArea {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 4; cursorShape: Qt.SizeVerCursor
            onPressed: floatWin.startSystemResize(Qt.TopEdge)
        }
        MouseArea {
            anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
            height: 4; cursorShape: Qt.SizeVerCursor
            onPressed: floatWin.startSystemResize(Qt.BottomEdge)
        }
        // Corners
        MouseArea {
            anchors { left: parent.left; top: parent.top }
            width: 6; height: 6; cursorShape: Qt.SizeFDiagCursor
            onPressed: floatWin.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
        }
        MouseArea {
            anchors { right: parent.right; top: parent.top }
            width: 6; height: 6; cursorShape: Qt.SizeBDiagCursor
            onPressed: floatWin.startSystemResize(Qt.RightEdge | Qt.TopEdge)
        }
        MouseArea {
            anchors { left: parent.left; bottom: parent.bottom }
            width: 6; height: 6; cursorShape: Qt.SizeBDiagCursor
            onPressed: floatWin.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
        }
        MouseArea {
            anchors { right: parent.right; bottom: parent.bottom }
            width: 6; height: 6; cursorShape: Qt.SizeFDiagCursor
            onPressed: floatWin.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
        }
    }
}
