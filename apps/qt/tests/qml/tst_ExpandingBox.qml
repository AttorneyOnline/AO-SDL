import QtQuick
import QtTest
import AO.Components

// Tests for ExpandingBox
//
// Covered:
//   - Default property values
//   - _expandsH / _expandsV derived flags (false when ≤ 0, true when > 0)
//   - _targetH when no button: button height is 0; target is 0 collapsed,
//     expandedHeight when expanded
//   - _targetW similarly for horizontal expansion
//   - Setting both expandedHeight and expandedWidth (bi-directional)
//   - invertHorizontal / invertVertical stored correctly
//   - animationDuration stored correctly
//   - expanded can be toggled programmatically
//   - implicitWidth/Height follow _expandsH/_expandsV and animated values

TestCase {
    name: "ExpandingBox"

    // ── Components ───────────────────────────────────────────────────────────
    Component {
        id: plainBox
        ExpandingBox {}
    }

    Component {
        id: vertBox
        // Expands vertically only; no button so _buttonH = 0.
        ExpandingBox { expandedHeight: 180; animationDuration: 0 }
    }

    Component {
        id: horizBox
        ExpandingBox { expandedWidth: 260; animationDuration: 0 }
    }

    Component {
        id: biDirBox
        ExpandingBox { expandedWidth: 300; expandedHeight: 200; animationDuration: 0 }
    }

    Component {
        id: invertedBox
        ExpandingBox {
            expandedHeight: 140
            invertHorizontal: true
            invertVertical:   true
            animationDuration: 0
        }
    }

    // ── Default property values ──────────────────────────────────────────────
    function test_defaults() {
        var b = createTemporaryObject(plainBox, this)
        verify(b !== null, "object created")
        compare(b.expanded,            false)
        compare(b.expandedHeight,      -1)
        compare(b.expandedWidth,       -1)
        compare(b.expandedMinimumSize, 60)
        compare(b.invertHorizontal,    false)
        compare(b.invertVertical,      false)
        compare(b.animationDuration,   200)
        compare(b.buttonComponent,     null)
    }

    // ── _expandsH / _expandsV ────────────────────────────────────────────────
    function test_expandsH_false_when_negativeOne() {
        var b = createTemporaryObject(plainBox, this)
        verify(!b._expandsH)
    }

    function test_expandsV_false_when_negativeOne() {
        var b = createTemporaryObject(plainBox, this)
        verify(!b._expandsV)
    }

    function test_expandsV_true_when_positiveHeight() {
        var b = createTemporaryObject(vertBox, this)
        verify(b._expandsV)
    }

    function test_expandsH_false_when_verticalOnly() {
        var b = createTemporaryObject(vertBox, this)
        verify(!b._expandsH)
    }

    function test_expandsH_true_when_positiveWidth() {
        var b = createTemporaryObject(horizBox, this)
        verify(b._expandsH)
    }

    function test_expandsV_false_when_horizontalOnly() {
        var b = createTemporaryObject(horizBox, this)
        verify(!b._expandsV)
    }

    function test_expandsH_and_V_both_true_biDir() {
        var b = createTemporaryObject(biDirBox, this)
        verify(b._expandsH)
        verify(b._expandsV)
    }

    // ── _expandsH/_expandsV update dynamically ───────────────────────────────
    function test_expandsV_updates_when_height_set() {
        var b = createTemporaryObject(plainBox, this)
        verify(!b._expandsV)
        b.expandedHeight = 150
        verify(b._expandsV)
        b.expandedHeight = -1
        verify(!b._expandsV)
    }

    function test_expandsH_updates_when_width_set() {
        var b = createTemporaryObject(plainBox, this)
        verify(!b._expandsH)
        b.expandedWidth = 200
        verify(b._expandsH)
    }

    // ── _targetH (no button → _buttonH = 0) ─────────────────────────────────
    function test_targetH_collapsed_isZero_noButton() {
        var b = createTemporaryObject(vertBox, this)
        // No button component → _buttonH = 0; collapsed → _targetH = 0.
        compare(b._targetH, 0)
    }

    function test_targetH_expanded_isExpandedHeight() {
        var b = createTemporaryObject(vertBox, this)
        b.expanded = true
        compare(b._targetH, 180)
    }

    function test_targetH_collapseAfterExpand_returnsToZero() {
        var b = createTemporaryObject(vertBox, this)
        b.expanded = true
        b.expanded = false
        compare(b._targetH, 0)
    }

    // ── _targetW (no button → _buttonW = 0) ─────────────────────────────────
    function test_targetW_collapsed_isZero_noButton() {
        var b = createTemporaryObject(horizBox, this)
        compare(b._targetW, 0)
    }

    function test_targetW_expanded_isExpandedWidth() {
        var b = createTemporaryObject(horizBox, this)
        b.expanded = true
        compare(b._targetW, 260)
    }

    // ── Bi-directional ───────────────────────────────────────────────────────
    function test_biDir_targetH_expanded() {
        var b = createTemporaryObject(biDirBox, this)
        b.expanded = true
        compare(b._targetH, 200)
        compare(b._targetW, 300)
    }

    function test_biDir_targetH_collapsed() {
        var b = createTemporaryObject(biDirBox, this)
        compare(b._targetH, 0)
        compare(b._targetW, 0)
    }

    // ── implicitWidth / implicitHeight reflect _expandsH/_V ─────────────────
    function test_implicitWidth_minusOne_when_notExpandingH() {
        var b = createTemporaryObject(vertBox, this)
        compare(b.implicitWidth, -1)
    }

    function test_implicitHeight_minusOne_when_notExpandingV() {
        var b = createTemporaryObject(horizBox, this)
        compare(b.implicitHeight, -1)
    }

    function test_implicitHeight_follows_animSize_when_expandingV() {
        var b = createTemporaryObject(vertBox, this)
        // animationDuration = 0 → _animH snaps to _targetH immediately.
        compare(b.implicitHeight, b._animH)
        b.expanded = true
        compare(b.implicitHeight, b._animH)
    }

    function test_implicitWidth_follows_animSize_when_expandingH() {
        var b = createTemporaryObject(horizBox, this)
        b.expanded = true
        compare(b.implicitWidth, b._animW)
    }

    // ── invertHorizontal / invertVertical ────────────────────────────────────
    function test_invertFlags_stored() {
        var b = createTemporaryObject(invertedBox, this)
        compare(b.invertHorizontal, true)
        compare(b.invertVertical,   true)
    }

    // ── animationDuration ────────────────────────────────────────────────────
    function test_animationDuration_stored() {
        var b = createTemporaryObject(plainBox, this)
        compare(b.animationDuration, 200)
    }

    function test_animationDuration_canBeChanged() {
        var b = createTemporaryObject(plainBox, this)
        b.animationDuration = 0
        compare(b.animationDuration, 0)
    }

    // ── expanded toggle ──────────────────────────────────────────────────────
    function test_expanded_toggle() {
        var b = createTemporaryObject(vertBox, this)
        compare(b.expanded, false)
        b.expanded = true
        compare(b.expanded, true)
        b.expanded = false
        compare(b.expanded, false)
    }
}
