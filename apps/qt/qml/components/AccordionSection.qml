import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * A single collapsible section for use inside an AccordionDrawer.
 *
 * AccordionSection handles its own open/close animation and header rendering.
 * Exclusivity (only one open at a time) is managed by the parent
 * AccordionDrawer — you do not need to implement that yourself.
 *
 * ── Normal usage (inside AccordionDrawer) ───────────────────────────────
 *
 *   AccordionDrawer {
 *       AccordionSection {
 *           title: "General"
 *
 *           ColumnLayout {
 *               anchors { left: parent.left; right: parent.right }
 *               anchors.margins: 8
 *               CheckBox { text: "Enable feature X" }
 *               Slider   { from: 0; to: 100 }
 *           }
 *       }
 *   }
 *
 * ── Standalone usage (manages its own open state) ───────────────────────
 *
 *   AccordionSection {
 *       title:    "Details"
 *       expanded: true          // starts open
 *       onToggled: expanded = !expanded   // wire up manually when standalone
 *
 *       Label { text: "Standalone section content" }
 *   }
 *
 * ── Disabling the open/close animation ──────────────────────────────────
 *
 *   AccordionSection {
 *       title: "Instant"
 *       animationDuration: 0    // snap open/closed with no transition
 *       …
 *   }
 *
 * ── Direction ────────────────────────────────────────────────────────────
 *
 * The `direction` property is usually inherited automatically from the
 * parent AccordionDrawer.  Only set it manually when using the section
 * standalone or in a custom container.  Use the AccordionSection.Direction
 * enum (identical values to AccordionDrawer.Direction):
 *
 *   AccordionSection.TopToBottom  — header on top, content expands downward  (default)
 *   AccordionSection.BottomToTop  — header on bottom, content expands upward
 *   AccordionSection.LeftToRight  — header on left, content expands rightward
 *   AccordionSection.RightToLeft  — header on right, content expands leftward
 *
 * ── Sizing content items ─────────────────────────────────────────────────
 *
 * Content items placed inside the section should fill their width for
 * vertical directions, and fill their height for horizontal directions.
 * Use anchors or Layout attached properties accordingly:
 *
 *   // Vertical direction — fill width:
 *   ColumnLayout { anchors { left: parent.left; right: parent.right } }
 *
 *   // Horizontal direction — fill height:
 *   ColumnLayout { anchors { top: parent.top; bottom: parent.bottom } }
 */
Item {
    id: root

    // ── Direction enum ─────────────────────────────────────────────────
    // Mirrors AccordionDrawer.Direction — values are intentionally identical
    // so the parent drawer can pass its int directly without conversion.
    enum Direction {
        TopToBottom,   // header at top,    content opens downward  (default)
        BottomToTop,   // header at bottom, content opens upward
        LeftToRight,   // header on left,   content opens rightward
        RightToLeft    // header on right,  content opens leftward
    }

    // ── Public API ─────────────────────────────────────────────────────

    /** Header text shown in the section bar. */
    property string title: ""

    /** Whether the content area is currently shown. */
    property bool expanded: false

    /** Animation duration in milliseconds.  Set to 0 to disable. */
    property int animationDuration: 200

    /** Opening direction.  Usually inherited from AccordionDrawer.  Use the AccordionSection.Direction enum. */
    property int direction: AccordionSection.TopToBottom

    /** Items placed inside the AccordionSection land in the content area. */
    default property alias content: contentArea.data

    /** Emitted when the user clicks the header to toggle this section. */
    signal toggled()

    // ── Derived flags ──────────────────────────────────────────────────
    readonly property bool _isHorizontal: direction === AccordionSection.LeftToRight || direction === AccordionSection.RightToLeft

    // ── Size animation ─────────────────────────────────────────────────
    readonly property real _headerSize: 28

    // Set by AccordionDrawer for horizontal mode: the width available for
    // content after all headers are accounted for.  -1 = standalone mode,
    // falls back to childrenRect.width.
    property real _drawerContentWidth: -1

    // The "natural" size of the content along the expanding axis.
    // Vertical: measured from childrenRect (children set their own height).
    // Horizontal: provided by the parent drawer, because children typically
    // anchor left/right and have no intrinsic width to measure.
    readonly property real _contentSize: _isHorizontal
        ? (_drawerContentWidth >= 0 ? _drawerContentWidth : contentArea.childrenRect.width)
        : contentArea.childrenRect.height

    readonly property real _targetSize: _headerSize + (expanded ? _contentSize : 0)

    property real _animSize: _headerSize

    Behavior on _animSize {
        NumberAnimation { duration: root.animationDuration; easing.type: Easing.OutCubic }
    }
    on_TargetSizeChanged: _animSize = _targetSize

    // Fill parent along the fixed axis; animate along the expanding axis.
    implicitWidth:  _isHorizontal ? _animSize          : (parent ? parent.width  : 200)
    implicitHeight: _isHorizontal ? (parent ? parent.height : 200) : _animSize

    clip: true

    // ── Arrow indicator ────────────────────────────────────────────────
    // Rows: TB, BT, LR, RL.  Columns: [expanded, collapsed].
    readonly property var _arrows: [
        ["\u25BE", "\u25B8"],   // TB:  ▾ / ▸
        ["\u25B4", "\u25B8"],   // BT:  ▴ / ▸
        ["\u25B8", "\u25BE"],   // LR:  ▸ / ▾
        ["\u25C2", "\u25BE"]    // RL:  ◂ / ▾
    ]
    readonly property string _arrowChar: _arrows[direction][expanded ? 0 : 1]

    // ── Header bar ─────────────────────────────────────────────────────
    Rectangle {
        id: header
        color: headerMouse.containsMouse ? "#363636" : "#2a2a2a"

        width:  root._isHorizontal ? root._headerSize : parent.width
        height: root._isHorizontal ? parent.height    : root._headerSize

        // Label for vertical modes (topToBottom / bottomToTop).
        RowLayout {
            visible: !root._isHorizontal
            anchors { fill: parent; leftMargin: 6; rightMargin: 6 }
            spacing: 4

            Label {
                text: root._arrowChar
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

        // Label for horizontal modes — rotated to read along the strip.
        Item {
            visible: root._isHorizontal
            anchors.fill: parent
            clip: true

            Row {
                width:  header.height
                height: root._headerSize
                anchors.centerIn: parent
                spacing: 4
                rotation: direction === AccordionSection.LeftToRight ? -90 : 90
                leftPadding:  6
                rightPadding: 6

                Label {
                    text: root._arrowChar
                    font.pixelSize: 10
                    color: "#999999"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Label {
                    text: root.title
                    font.bold: true
                    font.pixelSize: 11
                    color: "#cccccc"
                    elide: Text.ElideRight
                    width: header.height - 28
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        MouseArea {
            id: headerMouse
            objectName: "headerMouse"
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.toggled()
        }
    }

    // ── Content area ───────────────────────────────────────────────────
    Item {
        id: contentArea
        clip: true
        // Size and anchors set by the state machine below.
    }

    // ── Layout states ──────────────────────────────────────────────────
    // Anchors must be swapped via AnchorChanges — ternary bindings on
    // anchors do not reliably clear the previous anchor in QML.
    // PropertyChanges only sets the expanding-axis size; the cross axis
    // is fully determined by its two anchors.
    states: [
        State {
            name: "topToBottom"; when: direction === AccordionSection.TopToBottom
            AnchorChanges {
                target: header
                anchors { top: root.top; left: root.left; right: root.right; bottom: undefined }
            }
            AnchorChanges {
                target: contentArea
                anchors { top: header.bottom; left: root.left; right: root.right; bottom: undefined }
            }
            PropertyChanges { target: contentArea; height: root._contentSize }
        },
        State {
            name: "bottomToTop"; when: direction === AccordionSection.BottomToTop
            AnchorChanges {
                target: header
                anchors { bottom: root.bottom; left: root.left; right: root.right; top: undefined }
            }
            AnchorChanges {
                target: contentArea
                anchors { bottom: header.top; left: root.left; right: root.right; top: undefined }
            }
            PropertyChanges { target: contentArea; height: root._contentSize }
        },
        State {
            name: "leftToRight"; when: direction === AccordionSection.LeftToRight
            AnchorChanges {
                target: header
                anchors { left: root.left; top: root.top; bottom: root.bottom; right: undefined }
            }
            AnchorChanges {
                target: contentArea
                anchors { left: header.right; top: root.top; bottom: root.bottom; right: undefined }
            }
            PropertyChanges { target: contentArea; width: Math.max(root.width - root._headerSize, 0) }
        },
        State {
            name: "rightToLeft"; when: direction === AccordionSection.RightToLeft
            AnchorChanges {
                target: header
                anchors { right: root.right; top: root.top; bottom: root.bottom; left: undefined }
            }
            AnchorChanges {
                target: contentArea
                anchors { right: header.left; top: root.top; bottom: root.bottom; left: undefined }
            }
            PropertyChanges { target: contentArea; width: Math.max(root.width - root._headerSize, 0) }
        }
    ]
}
