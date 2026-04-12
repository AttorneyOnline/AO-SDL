import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Wrapper that lets any panel be popped out into a floating tool window.
 *
 * Usage inside a SplitView:
 *
 *   DockPanel {
 *       title:             "Music & Areas"
 *       dockedHeight:      200          // how tall when docked (default 200)
 *       dockedMinimumHeight: 80         // minimum resize height (default 60)
 *       undockedSize:      Qt.size(280, 400)
 *
 *       panelComponent: Component {
 *           MusicAreaPanel { controller: root.controller }
 *       }
 *   }
 *
 * Works in SplitView, ColumnLayout, RowLayout, or a plain Item — the
 * component sets the right attached properties for each automatically.
 * Callers should not set SplitView.* or Layout.* height properties on
 * this item.  When undocked, it collapses to a slim re-dock strip so
 * sibling panels can reclaim the space.
 *
 * Usage inside a ColumnLayout:
 *
 *   ColumnLayout {
 *       DockPanel {
 *           title:        "Chat"
 *           dockedHeight: 180
 *           panelComponent: Component { ChatPanel { } }
 *       }
 *   }
 *
 * Usage as a floating overlay (outside a layout):
 *
 *   DockPanel {
 *       title:        "Emotes"
 *       visible:      false
 *       overlay:      true              // disables SplitView collapse behaviour
 *       undockedSize: Qt.size(400, 300)
 *       width: 400; height: 300
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

    // SplitView parent
    SplitView.preferredHeight: overlay ? undefined : _currentHeight
    SplitView.minimumHeight:   overlay ? undefined
                             : (docked ? dockedMinimumHeight : _collapsedHeight)
    SplitView.maximumHeight:   overlay ? undefined
                             : (docked ? Number.POSITIVE_INFINITY : _collapsedHeight)

    // ColumnLayout / RowLayout parent
    Layout.preferredHeight: overlay ? undefined : _currentHeight
    Layout.minimumHeight:   overlay ? undefined
                          : (docked ? dockedMinimumHeight : _collapsedHeight)
    Layout.maximumHeight:   overlay ? undefined
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
