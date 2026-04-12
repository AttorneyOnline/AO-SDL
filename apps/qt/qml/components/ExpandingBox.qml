import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Collapsible container that toggles between a compact button and an
 * expanded content area along a configurable axis.
 *
 * Vertical usage (default, expands downward):
 *
 *   ExpandingBox {
 *       expandedSize: 180
 *       buttonComponent: Component { Button { text: "Filters \u25BE" } }
 *
 *       CheckBox { text: "Option A" }
 *       CheckBox { text: "Option B" }
 *   }
 *
 * Horizontal usage (expands rightward):
 *
 *   ExpandingBox {
 *       orientation:  Qt.Horizontal
 *       expandedSize: 260
 *       buttonComponent: Component { ToolButton { text: "\u25B8" } }
 *
 *       ListView { anchors.fill: parent; model: myModel }
 *   }
 *
 * Collapsed: only the toggle button is visible, the item shrinks to the
 * button size along the expand axis, releasing space to siblings.
 *
 * Expanded: the content area appears beside the button, constrained to
 * expandedSize along the expand axis.
 *
 * Works in SplitView, ColumnLayout, RowLayout, or a plain Item — the
 * component sets the right attached properties for each automatically.
 */
Item {
    id: root

    // ── Public API ─────────────────────────────────────────────────────

    /** Expand direction.  Qt.Vertical (default) or Qt.Horizontal. */
    property int orientation: Qt.Vertical

    /** Whether the content area is currently shown. */
    property bool expanded: false

    /** Size along the expand axis when expanded (height or width). */
    property real expandedSize: 200

    /** Minimum size along the expand axis when expanded. */
    property real expandedMinimumSize: 60

    /** Component instantiated as the toggle button.  The loaded item
     *  receives no implicit sizing — the caller controls its look.
     *  Clicking the button toggles `expanded`; connect additional
     *  signals as needed. */
    property Component buttonComponent: null

    /** Items placed inside the ExpandingBox land in the content area. */
    default property alias content: contentArea.data

    // ── Internals ──────────────────────────────────────────────────────
    readonly property bool _vertical: orientation === Qt.Vertical
    readonly property real _buttonSize: _vertical
                                      ? buttonLoader.implicitHeight
                                      : buttonLoader.implicitWidth
    readonly property real _currentSize: expanded ? expandedSize : _buttonSize

    // ── Universal sizing ───────────────────────────────────────────────
    implicitHeight: _vertical ? _currentSize : expandedSize
    implicitWidth:  _vertical ? expandedSize : _currentSize

    // ── SplitView parent ───────────────────────────────────────────────
    SplitView.preferredHeight: _vertical ? _currentSize : undefined
    SplitView.minimumHeight:   _vertical ? (expanded ? expandedMinimumSize : _buttonSize) : undefined
    SplitView.maximumHeight:   _vertical ? (expanded ? Number.POSITIVE_INFINITY : _buttonSize) : undefined

    SplitView.preferredWidth:  _vertical ? undefined : _currentSize
    SplitView.minimumWidth:    _vertical ? undefined : (expanded ? expandedMinimumSize : _buttonSize)
    SplitView.maximumWidth:    _vertical ? undefined : (expanded ? Number.POSITIVE_INFINITY : _buttonSize)

    // ── Layout parent ──────────────────────────────────────────────────
    Layout.preferredHeight: _vertical ? _currentSize : undefined
    Layout.minimumHeight:   _vertical ? (expanded ? expandedMinimumSize : _buttonSize) : undefined
    Layout.maximumHeight:   _vertical ? (expanded ? Number.POSITIVE_INFINITY : _buttonSize) : undefined
    Layout.fillWidth:       _vertical

    Layout.preferredWidth:  _vertical ? undefined : _currentSize
    Layout.minimumWidth:    _vertical ? undefined : (expanded ? expandedMinimumSize : _buttonSize)
    Layout.maximumWidth:    _vertical ? undefined : (expanded ? Number.POSITIVE_INFINITY : _buttonSize)
    Layout.fillHeight:      !_vertical

    // ── Toggle button ──────────────────────────────────────────────────
    Loader {
        id: buttonLoader
        sourceComponent: root.buttonComponent
        // Anchor to the leading edge along the expand axis.
        anchors {
            top:  parent.top
            left: parent.left
        }
        width:  root._vertical ? parent.width  : implicitWidth
        height: root._vertical ? implicitHeight : parent.height

        // Forward clicks from the loaded item to toggle expanded.
        Connections {
            target: buttonLoader.item
            function onClicked() { root.expanded = !root.expanded }
        }
    }

    // ── Content area ───────────────────────────────────────────────────
    Item {
        id: contentArea
        visible: root.expanded
        clip: true
        anchors {
            top:    root._vertical ? buttonLoader.bottom : parent.top
            left:   root._vertical ? parent.left : buttonLoader.right
            right:  parent.right
            bottom: parent.bottom
        }
    }
}
