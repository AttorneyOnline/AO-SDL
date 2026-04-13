import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Collapsible container that toggles between a compact button and an
 * expanded content area, with a smooth animation.
 *
 * Provide a `buttonComponent` — any clickable QML item — and place content
 * inside the ExpandingBox as normal children.  Clicking the button toggles
 * `expanded` automatically; no signal wiring is required.
 *
 * Set `expandedHeight`, `expandedWidth`, or both to control which axes grow.
 * Leave an axis at -1 (default) to keep it at its natural size and defer
 * sizing on that axis to the parent layout.
 *
 * ExpandingBox sets the correct SplitView.*, Layout.*, and implicit size
 * properties automatically for every supported parent type.
 *
 * ── Vertical — expands downward ─────────────────────────────────────────
 *
 *   ExpandingBox {
 *       expandedHeight: 180
 *       buttonComponent: Component { Button { text: "Filters" } }
 *
 *       ColumnLayout {
 *           anchors.fill: parent
 *           anchors.margins: 4
 *           CheckBox { text: "Show taken"; checked: true }
 *           CheckBox { text: "Favourites only" }
 *       }
 *   }
 *
 * ── Horizontal — expands rightward ──────────────────────────────────────
 *
 *   ExpandingBox {
 *       expandedWidth: 260
 *       buttonComponent: Component { ToolButton { text: "\u25B8" } }
 *
 *       ListView {
 *           anchors.fill: parent
 *           model: playerModel
 *           delegate: ItemDelegate { width: ListView.view.width; text: modelData }
 *       }
 *   }
 *
 * ── Bi-directional — expands to bottom-right ────────────────────────────
 *
 *   ExpandingBox {
 *       expandedWidth:  300
 *       expandedHeight: 200
 *       buttonComponent: Component { Button { text: "Evidence" } }
 *
 *       GridLayout { anchors.fill: parent; anchors.margins: 6; columns: 3
 *           Repeater { model: 9; delegate: Rectangle { width: 72; height: 72 } }
 *       }
 *   }
 *
 * ── Inverted — expands leftward / upward ────────────────────────────────
 *
 * Use `invertHorizontal` or `invertVertical` when the button sits on the
 * far edge and the content should open toward the centre of the screen.
 *
 *   // Content grows leftward from a right-edge button:
 *   ExpandingBox {
 *       expandedWidth:    260
 *       invertHorizontal: true
 *       buttonComponent: Component { ToolButton { text: "\u25C0" } }
 *       Label { text: "Right-edge panel" }
 *   }
 *
 *   // Content grows upward from a bottom button:
 *   ExpandingBox {
 *       expandedHeight: 140
 *       invertVertical:  true
 *       buttonComponent: Component { Button { text: "\u25B2 History" } }
 *       ListView { anchors.fill: parent; model: historyModel }
 *   }
 *
 * ── Controlling expanded state from code ────────────────────────────────
 *
 *   ExpandingBox {
 *       id: filterBox
 *       expandedHeight: 160
 *       expanded: true          // start open
 *       …
 *   }
 *
 *   Button { text: "Toggle filters"; onClicked: filterBox.expanded = !filterBox.expanded }
 *
 * ── Disabling animation ──────────────────────────────────────────────────
 *
 *   ExpandingBox {
 *       animationDuration: 0    // instant open/close, no transition
 *       …
 *   }
 */
Item {
    id: root

    // ── Public API ─────────────────────────────────────────────────────

    /** Whether the content area is currently shown. */
    property bool expanded: false

    /** Expanded height.  Set to -1 (default) to not expand vertically. */
    property real expandedHeight: -1

    /** Expanded width.  Set to -1 (default) to not expand horizontally. */
    property real expandedWidth: -1

    /** Minimum expanded size on the active axis (for resizable layouts). */
    property real expandedMinimumSize: 60

    /** Flip horizontal expansion to grow leftward (button on the right). */
    property bool invertHorizontal: false

    /** Flip vertical expansion to grow upward (button on the bottom). */
    property bool invertVertical: false

    /** Animation duration in milliseconds.  Set to 0 to disable. */
    property int animationDuration: 200

    /** Component instantiated as the toggle button.  Clicking it
     *  toggles `expanded` automatically. */
    property Component buttonComponent: null

    /** Items placed inside the ExpandingBox land in the content area. */
    default property alias content: contentArea.data

    // ── Internals ──────────────────────────────────────────────────────
    readonly property bool _expandsH: expandedWidth  > 0
    readonly property bool _expandsV: expandedHeight > 0

    readonly property real _buttonW: buttonLoader.implicitWidth
    readonly property real _buttonH: buttonLoader.implicitHeight

    readonly property real _targetW: (_expandsH && expanded) ? expandedWidth  : _buttonW
    readonly property real _targetH: (_expandsV && expanded) ? expandedHeight : _buttonH

    readonly property bool _animating: _animW !== _targetW || _animH !== _targetH

    property real _animW: _buttonW
    property real _animH: _buttonH

    Behavior on _animW {
        NumberAnimation { duration: root.animationDuration; easing.type: Easing.OutCubic }
    }
    Behavior on _animH {
        NumberAnimation { duration: root.animationDuration; easing.type: Easing.OutCubic }
    }
    on_TargetWChanged: _animW = _targetW
    on_TargetHChanged: _animH = _targetH

    // ── Universal sizing ───────────────────────────────────────────────
    implicitWidth:  _expandsH ? _animW : -1
    implicitHeight: _expandsV ? _animH : -1

    // ── SplitView parent ───────────────────────────────────────────────
    SplitView.preferredHeight: _expandsV ? _animH : -1
    SplitView.minimumHeight:   _expandsV ? _animH : -1
    SplitView.maximumHeight:   _expandsV ? _animH : -1

    SplitView.preferredWidth:  _expandsH ? _animW : -1
    SplitView.minimumWidth:    _expandsH ? _animW : -1
    SplitView.maximumWidth:    _expandsH ? _animW : -1

    // ── Layout parent ──────────────────────────────────────────────────
    Layout.preferredHeight: _expandsV ? _animH : -1
    Layout.minimumHeight:   _expandsV ? _animH : -1
    Layout.maximumHeight:   _expandsV ? _animH : -1
    Layout.fillWidth:       !_expandsH

    Layout.preferredWidth:  _expandsH ? _animW : -1
    Layout.minimumWidth:    _expandsH ? _animW : -1
    Layout.maximumWidth:    _expandsH ? _animW : -1
    Layout.fillHeight:      !_expandsV

    // ── Helpers for anchor selection ──────────────────────────────────
    // Whether the button acts as a horizontal bar (spans width) or
    // vertical bar (spans height).
    readonly property bool _buttonIsBar: _expandsV || !_expandsH

    // ── Toggle button ──────────────────────────────────────────────────
    Loader {
        id: buttonLoader
        sourceComponent: root.buttonComponent

        // Button sits at the edge opposite the expand direction.
        anchors.top:    root._buttonIsBar && !root.invertVertical   ? parent.top    : undefined
        anchors.bottom: root._buttonIsBar &&  root.invertVertical   ? parent.bottom : undefined
        anchors.left:  !root._buttonIsBar && !root.invertHorizontal ? parent.left   : undefined
        anchors.right: !root._buttonIsBar &&  root.invertHorizontal ? parent.right  : undefined

        // Bar axis: span the full cross-dimension.
        // Vertical/bi-directional: horizontal bar spanning width.
        // Horizontal only: vertical bar spanning height.
        width:  root._buttonIsBar ? parent.width  : implicitWidth
        height: root._buttonIsBar ? implicitHeight : parent.height

        Connections {
            target: buttonLoader.item
            function onClicked() { root.expanded = !root.expanded }
        }
    }

    // ── Content area ───────────────────────────────────────────────────
    Item {
        id: contentArea
        visible: root.expanded || root._animating
        clip: true

        // Content fills the space on the expand side of the button.
        anchors.top:    root._buttonIsBar
                        ? (root.invertVertical ? parent.top : buttonLoader.bottom)
                        : parent.top
        anchors.bottom: root._buttonIsBar
                        ? (root.invertVertical ? buttonLoader.top : parent.bottom)
                        : parent.bottom
        anchors.left:   root._buttonIsBar
                        ? parent.left
                        : (root.invertHorizontal ? parent.left : buttonLoader.right)
        anchors.right:  root._buttonIsBar
                        ? parent.right
                        : (root.invertHorizontal ? buttonLoader.left : parent.right)
    }
}
