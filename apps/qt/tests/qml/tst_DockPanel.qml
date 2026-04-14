import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtTest
import AO.Components

// Tests for DockPanel
//
// Covered:
//   - Default property values
//   - _collapsedHeight constant (22 px)
//   - _currentHeight logic:
//       docked=true,  overlay=false → dockedHeight
//       docked=false, overlay=false → _collapsedHeight (22)
//       docked=false, overlay=true  → dockedHeight  (overlay ignores undocked)
//       docked=true,  overlay=true  → dockedHeight
//   - implicitHeight always mirrors _currentHeight
//   - SplitView.preferredHeight follows _currentHeight (non-overlay path)
//   - Layout.preferredHeight follows _currentHeight (non-overlay path)
//   - dockedHeight changes are reflected in _currentHeight while docked
//   - title, overlay, dockedMinimumHeight, undockedSize are stored correctly
//   - docked can be toggled programmatically

TestCase {
    name: "DockPanel"

    // ── Components ───────────────────────────────────────────────────────────
    Component {
        id: plainPanel
        DockPanel {}
    }

    Component {
        id: customHeightPanel
        DockPanel { dockedHeight: 350 }
    }

    Component {
        id: overlayPanel
        DockPanel { overlay: true; dockedHeight: 180 }
    }

    Component {
        id: titledPanel
        DockPanel { title: "Music & Areas"; dockedHeight: 220; dockedMinimumHeight: 80 }
    }

    Component {
        id: customUndockedPanel
        DockPanel { undockedSize: Qt.size(480, 360) }
    }

    // ── Default property values ──────────────────────────────────────────────
    function test_defaults() {
        var p = createTemporaryObject(plainPanel, this)
        verify(p !== null, "object created")
        compare(p.title,               "")
        compare(p.docked,              true)
        compare(p.overlay,             false)
        compare(p.dockedHeight,        200)
        compare(p.dockedMinimumHeight, 60)
        compare(p.undockedSize,        Qt.size(300, 240))
        compare(p.panelComponent,      null)
    }

    // ── _collapsedHeight constant ────────────────────────────────────────────
    function test_collapsedHeight_is22() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p._collapsedHeight, 22)
    }

    // ── _currentHeight logic ─────────────────────────────────────────────────
    function test_currentHeight_docked_equalsDockedHeight() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p._currentHeight, p.dockedHeight)   // 200
    }

    function test_currentHeight_undocked_equalsCollapsedHeight() {
        var p = createTemporaryObject(plainPanel, this)
        p.docked = false
        compare(p._currentHeight, p._collapsedHeight)   // 22
    }

    function test_currentHeight_overlay_docked_equalsDockedHeight() {
        var p = createTemporaryObject(overlayPanel, this)
        compare(p._currentHeight, p.dockedHeight)   // 180
    }

    function test_currentHeight_overlay_undocked_stillEqualsDockedHeight() {
        // With overlay: true, _currentHeight ignores the docked flag.
        var p = createTemporaryObject(overlayPanel, this)
        p.docked = false
        compare(p._currentHeight, p.dockedHeight)   // 180, not 22
    }

    function test_currentHeight_customDockedHeight() {
        var p = createTemporaryObject(customHeightPanel, this)
        compare(p._currentHeight, 350)
    }

    // ── implicitHeight mirrors _currentHeight ────────────────────────────────
    function test_implicitHeight_docked() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p.implicitHeight, p._currentHeight)
    }

    function test_implicitHeight_undocked() {
        var p = createTemporaryObject(plainPanel, this)
        p.docked = false
        compare(p.implicitHeight, p._currentHeight)
    }

    // ── SplitView attached properties ────────────────────────────────────────
    function test_splitViewPreferredHeight_docked_equalsCurrentHeight() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p.SplitView.preferredHeight, p._currentHeight)
    }

    function test_splitViewPreferredHeight_undocked_equalsCollapsedHeight() {
        var p = createTemporaryObject(plainPanel, this)
        p.docked = false
        compare(p.SplitView.preferredHeight, p._collapsedHeight)
    }

    function test_splitViewPreferredHeight_overlay_isUnset() {
        var p = createTemporaryObject(overlayPanel, this)
        // overlay: true → SplitView.preferredHeight: -1 ("no preference" sentinel)
        compare(p.SplitView.preferredHeight, -1)
    }

    // ── Layout attached properties ───────────────────────────────────────────
    function test_layoutPreferredHeight_docked_equalsCurrentHeight() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p.Layout.preferredHeight, p._currentHeight)
    }

    function test_layoutPreferredHeight_undocked_equalsCollapsedHeight() {
        var p = createTemporaryObject(plainPanel, this)
        p.docked = false
        compare(p.Layout.preferredHeight, p._collapsedHeight)
    }

    // ── dockedHeight change while docked ────────────────────────────────────
    function test_dockedHeight_change_updatesCurrentHeight() {
        var p = createTemporaryObject(plainPanel, this)
        p.dockedHeight = 400
        compare(p._currentHeight, 400)
        compare(p.implicitHeight,  400)
    }

    // ── Misc stored properties ───────────────────────────────────────────────
    function test_title_isStored() {
        var p = createTemporaryObject(titledPanel, this)
        compare(p.title, "Music & Areas")
    }

    function test_dockedMinimumHeight_isStored() {
        var p = createTemporaryObject(titledPanel, this)
        compare(p.dockedMinimumHeight, 80)
    }

    function test_undockedSize_isStored() {
        var p = createTemporaryObject(customUndockedPanel, this)
        compare(p.undockedSize, Qt.size(480, 360))
    }

    // ── docked toggle ────────────────────────────────────────────────────────
    function test_docked_toggle() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p.docked, true)
        p.docked = false
        compare(p.docked, false)
        p.docked = true
        compare(p.docked, true)
    }
}
