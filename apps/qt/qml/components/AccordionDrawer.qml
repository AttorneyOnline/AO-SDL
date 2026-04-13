import QtQuick
import QtQuick.Layouts

/**
 * Exclusive-expand container for AccordionSection items.
 *
 * Only one section can be expanded at a time.  Expanding a section
 * automatically collapses any previously open one.
 *
 * Basic usage:
 *
 *   AccordionDrawer {
 *       anchors.fill: parent
 *
 *       AccordionSection {
 *           title: "General"
 *           Label { text: "General content" }
 *       }
 *       AccordionSection {
 *           title: "Audio"
 *           Slider { from: 0; to: 100 }
 *       }
 *       AccordionSection {
 *           title: "Advanced"
 *           CheckBox { text: "Enable feature X" }
 *       }
 *   }
 *
 * Controlling from code:
 *
 *   AccordionDrawer {
 *       id: drawer
 *       expandedIndex: 0          // first section starts open
 *   }
 *   Button { text: "Close all"; onClicked: drawer.expandedIndex = -1 }
 *
 * Inside a SplitView or Layout the drawer fills its allocated space.
 * Sections are stacked at the top; remaining space is empty.
 */
Item {
    id: root

    // ── Public API ─────────────────────────────────────────────────────

    /** Index of the currently expanded section, or -1 if all collapsed. */
    property int expandedIndex: -1

    /** Section items are placed inside the internal column. */
    default property alias sections: column.data

    // ── Layout integration ─────────────────────────────────────────────
    Layout.fillWidth:  true
    Layout.fillHeight: true

    // ── Internal column ────────────────────────────────────────────────
    Column {
        id: column
        anchors { left: parent.left; right: parent.right; top: parent.top }
    }

    // ── Wiring ─────────────────────────────────────────────────────────
    onExpandedIndexChanged: _syncSections()

    /** Scan children and connect toggle signals.  Called once after all
     *  sections are instantiated, and again if children change. */
    Component.onCompleted: Qt.callLater(_wireUp)
    onSectionsChanged:     Qt.callLater(_wireUp)

    property var _wiredSections: []
    property var _toggleCallbacks: []

    function _wireUp() {
        // Disconnect previous connections
        for (let i = 0; i < _wiredSections.length; i++) {
            let prev = _wiredSections[i]
            if (prev && prev.toggled && _toggleCallbacks[i])
                prev.toggled.disconnect(_toggleCallbacks[i])
        }

        let wired = []
        let callbacks = []
        for (let i = 0; i < column.children.length; i++) {
            let child = column.children[i]
            if (!child || child.toggled === undefined)
                continue

            // Capture index in closure
            const idx = wired.length
            let cb = function() {
                root.expandedIndex = (root.expandedIndex === idx) ? -1 : idx
            }
            child.toggled.connect(cb)
            wired.push(child)
            callbacks.push(cb)
        }

        _wiredSections = wired
        _toggleCallbacks = callbacks
        _syncSections()
    }

    function _syncSections() {
        for (let i = 0; i < _wiredSections.length; i++) {
            _wiredSections[i].expanded = (i === expandedIndex)
        }
    }
}
