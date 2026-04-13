import QtQuick
import QtQuick.Layouts

/**
 * Exclusive-expand container for AccordionSection items.
 *
 * Only one section can be expanded at a time — clicking an open header
 * collapses it; clicking a closed header opens it and collapses whichever
 * was previously open.
 *
 * ── Basic usage (default top-to-bottom) ─────────────────────────────────
 *
 *   AccordionDrawer {
 *       anchors.fill: parent
 *
 *       AccordionSection { title: "General";  Label { text: "…" }  }
 *       AccordionSection { title: "Audio";    Slider { … }         }
 *       AccordionSection { title: "Advanced"; CheckBox { … }       }
 *   }
 *
 * ── Choosing a direction ────────────────────────────────────────────────
 *
 *   AccordionDrawer {
 *       direction: AccordionDrawer.LeftToRight    // sections open rightward
 *       …
 *   }
 *
 * Available directions (AccordionDrawer.Direction enum):
 *   AccordionDrawer.TopToBottom  — headers stacked at top, content opens downward  (default)
 *   AccordionDrawer.BottomToTop  — headers stacked at bottom, content opens upward
 *   AccordionDrawer.LeftToRight  — headers stacked on left, content opens rightward
 *   AccordionDrawer.RightToLeft  — headers stacked on right, content opens leftward
 *
 * The `direction` is automatically propagated to every child AccordionSection,
 * so you never need to set it on individual sections.
 *
 * ── Controlling the open section from code ──────────────────────────────
 *
 *   AccordionDrawer {
 *       id: drawer
 *       expandedIndex: 0          // first section starts open
 *   }
 *
 *   Button { text: "Open Audio";  onClicked: drawer.expandedIndex = 1  }
 *   Button { text: "Close all";   onClicked: drawer.expandedIndex = -1 }
 *
 * ── Inside a Layout ─────────────────────────────────────────────────────
 *
 *   ColumnLayout {
 *       AccordionDrawer {
 *           Layout.fillWidth:  true
 *           Layout.fillHeight: true   // takes remaining space in the column
 *           …
 *       }
 *   }
 *
 * ── Inside a SplitView ──────────────────────────────────────────────────
 *
 *   SplitView {
 *       orientation: Qt.Horizontal
 *
 *       AccordionDrawer {
 *           SplitView.preferredWidth: 220
 *           direction: AccordionDrawer.LeftToRight
 *           …
 *       }
 *
 *       Item { SplitView.fillWidth: true }   // main content area
 *   }
 */
Item {
    id: root

    // ── Direction enum ─────────────────────────────────────────────────
    enum Direction {
        TopToBottom,   // headers at top,    content opens downward  (default)
        BottomToTop,   // headers at bottom, content opens upward
        LeftToRight,   // headers on left,   content opens rightward
        RightToLeft    // headers on right,  content opens leftward
    }

    // ── Public API ─────────────────────────────────────────────────────

    /** Index of the currently expanded section, or -1 if all collapsed. */
    property int expandedIndex: -1

    /** Direction in which sections open.  Use the AccordionDrawer.Direction enum. */
    property int direction: AccordionDrawer.TopToBottom

    /**
     * Declared AccordionSection items land here first, then are moved
     * to the active layout container by _wireUp().
     */
    default property alias sections: _staging.data

    // ── Layout integration ─────────────────────────────────────────────
    Layout.fillWidth:  true
    Layout.fillHeight: true

    // ── Internal flags ─────────────────────────────────────────────────
    readonly property bool _isHorizontal: direction === AccordionDrawer.LeftToRight || direction === AccordionDrawer.RightToLeft
    readonly property bool _isReverse:    direction === AccordionDrawer.BottomToTop  || direction === AccordionDrawer.RightToLeft

    // Width available for content in horizontal mode (total width minus all headers).
    readonly property real _horizontalContentWidth: _isHorizontal
        ? Math.max(width - _wiredSections.length * 28, 0)
        : -1

    // ── Staging area — children are reparented to the active layout ────
    Item {
        id: _staging
        visible: false
        width: 0; height: 0
    }

    // ── Vertical layout (topToBottom / bottomToTop) ────────────────────
    Column {
        id: vLayout
        visible: !root._isHorizontal
        anchors {
            left:   parent.left
            right:  parent.right
            top:    root._isReverse ? undefined : parent.top
            bottom: root._isReverse ? parent.bottom : undefined
        }
    }

    // ── Horizontal layout (leftToRight / rightToLeft) ──────────────────
    Row {
        id: hLayout
        visible: root._isHorizontal
        anchors {
            top:    parent.top
            bottom: parent.bottom
            left:   root._isReverse ? undefined : parent.left
            right:  root._isReverse ? parent.right : undefined
        }
    }

    readonly property Item _activeLayout: root._isHorizontal ? hLayout : vLayout

    // ── Wiring ─────────────────────────────────────────────────────────
    onExpandedIndexChanged: _syncSections()
    on_HorizontalContentWidthChanged: _syncSections()

    Component.onCompleted: Qt.callLater(_wireUp)
    onSectionsChanged:     Qt.callLater(_wireUp)

    property var _wiredSections:   []
    property var _toggleCallbacks: []

    function _wireUp() {
        // Move any staged children into the active layout and set direction.
        let staged = []
        for (let i = 0; i < _staging.children.length; i++)
            staged.push(_staging.children[i])
        for (let i = 0; i < staged.length; i++) {
            let s = staged[i]
            s.parent = root._activeLayout
            if (s.direction !== undefined)
                s.direction = root.direction
            if (s._drawerContentWidth !== undefined)
                s._drawerContentWidth = root._horizontalContentWidth
        }

        // Propagate direction to children already in the active layout.
        for (let i = 0; i < root._activeLayout.children.length; i++) {
            let c = root._activeLayout.children[i]
            if (c && c.direction !== undefined)
                c.direction = root.direction
        }

        // Disconnect previous toggle connections.
        for (let i = 0; i < _wiredSections.length; i++) {
            let prev = _wiredSections[i]
            if (prev && prev.toggled && _toggleCallbacks[i])
                prev.toggled.disconnect(_toggleCallbacks[i])
        }

        let wired     = []
        let callbacks = []
        for (let i = 0; i < root._activeLayout.children.length; i++) {
            let child = root._activeLayout.children[i]
            if (!child || child.toggled === undefined)
                continue

            const idx = wired.length
            let cb = function() {
                root.expandedIndex = (root.expandedIndex === idx) ? -1 : idx
            }
            child.toggled.connect(cb)
            wired.push(child)
            callbacks.push(cb)
        }

        _wiredSections   = wired
        _toggleCallbacks = callbacks
        _syncSections()
    }

    function _syncSections() {
        for (let i = 0; i < _wiredSections.length; i++) {
            _wiredSections[i].expanded = (i === expandedIndex)
            if (_wiredSections[i]._drawerContentWidth !== undefined)
                _wiredSections[i]._drawerContentWidth = root._horizontalContentWidth
        }
    }
}
