import QtQuick
import QtTest
import AO.Components

// Tests for AccordionDrawer
//
// Covered:
//   - Default property values (expandedIndex = -1, direction = TopToBottom)
//   - Direction enum integer values (shared with AccordionSection, 0–3)
//   - _isHorizontal and _isReverse derived flags for all four directions
//   - After wiring, _wiredSections contains the child AccordionSections
//   - Setting expandedIndex programmatically expands the correct section
//     and collapses all others
//   - expandedIndex = -1 collapses every section
//   - Emitting toggled on a section toggles expandedIndex correctly
//   - Direction is propagated to child sections after wiring

TestCase {
    name: "AccordionDrawer"
    when: windowShown

    // ── Components ───────────────────────────────────────────────────────────
    Component {
        id: emptyDrawer
        AccordionDrawer { width: 200; height: 300 }
    }

    Component {
        id: drawerTwo
        AccordionDrawer {
            width: 200; height: 300
            AccordionSection { title: "A"; animationDuration: 0 }
            AccordionSection { title: "B"; animationDuration: 0 }
        }
    }

    Component {
        id: drawerThree
        AccordionDrawer {
            width: 200; height: 400
            AccordionSection { title: "X"; animationDuration: 0 }
            AccordionSection { title: "Y"; animationDuration: 0 }
            AccordionSection { title: "Z"; animationDuration: 0 }
        }
    }

    Component {
        id: drawerLTR
        AccordionDrawer {
            direction: AccordionDrawer.LeftToRight
            width: 400; height: 200
            AccordionSection { title: "A"; animationDuration: 0 }
        }
    }

    Component {
        id: drawerBTT
        AccordionDrawer {
            direction: AccordionDrawer.BottomToTop
            width: 200; height: 300
            AccordionSection { title: "A"; animationDuration: 0 }
        }
    }

    Component {
        id: drawerRTL
        AccordionDrawer {
            direction: AccordionDrawer.RightToLeft
            width: 400; height: 200
            AccordionSection { title: "A"; animationDuration: 0 }
        }
    }

    // Helper: create a drawer and wait for Qt.callLater(_wireUp) to fire.
    function makeWired(comp) {
        var d = createTemporaryObject(comp, this)
        verify(d !== null, "object created")
        wait(50)   // let callLater(_wireUp) complete
        return d
    }

    // ── Default properties ───────────────────────────────────────────────────
    function test_defaults() {
        var d = createTemporaryObject(emptyDrawer, this)
        compare(d.expandedIndex, -1)
        compare(d.direction,     AccordionDrawer.TopToBottom)
    }

    // ── Direction enum values ────────────────────────────────────────────────
    function test_enum_TopToBottom() { compare(AccordionDrawer.TopToBottom, 0) }
    function test_enum_BottomToTop() { compare(AccordionDrawer.BottomToTop, 1) }
    function test_enum_LeftToRight() { compare(AccordionDrawer.LeftToRight, 2) }
    function test_enum_RightToLeft() { compare(AccordionDrawer.RightToLeft, 3) }

    // ── _isHorizontal derived flag ───────────────────────────────────────────
    function test_isHorizontal_TopToBottom_isFalse() {
        var d = createTemporaryObject(emptyDrawer, this)
        verify(!d._isHorizontal)
    }
    function test_isHorizontal_LeftToRight_isTrue() {
        var d = createTemporaryObject(drawerLTR, this)
        verify(d._isHorizontal)
    }
    function test_isHorizontal_RightToLeft_isTrue() {
        var d = createTemporaryObject(drawerRTL, this)
        verify(d._isHorizontal)
    }
    function test_isHorizontal_BottomToTop_isFalse() {
        var d = createTemporaryObject(drawerBTT, this)
        verify(!d._isHorizontal)
    }

    // ── _isReverse derived flag ──────────────────────────────────────────────
    function test_isReverse_TopToBottom_isFalse() {
        var d = createTemporaryObject(emptyDrawer, this)
        verify(!d._isReverse)
    }
    function test_isReverse_BottomToTop_isTrue() {
        var d = createTemporaryObject(drawerBTT, this)
        verify(d._isReverse)
    }
    function test_isReverse_LeftToRight_isFalse() {
        var d = createTemporaryObject(drawerLTR, this)
        verify(!d._isReverse)
    }
    function test_isReverse_RightToLeft_isTrue() {
        var d = createTemporaryObject(drawerRTL, this)
        verify(d._isReverse)
    }

    // ── Section wiring ───────────────────────────────────────────────────────
    function test_wiredSections_count_matchesChildren() {
        var d = makeWired(drawerTwo)
        compare(d._wiredSections.length, 2)
    }

    function test_wiredSections_count_three() {
        var d = makeWired(drawerThree)
        compare(d._wiredSections.length, 3)
    }

    function test_emptyDrawer_wiredSections_isEmpty() {
        var d = makeWired(emptyDrawer)
        compare(d._wiredSections.length, 0)
    }

    // ── expandedIndex controls section.expanded ──────────────────────────────
    function test_expandedIndex_minusOne_allCollapsed() {
        var d = makeWired(drawerTwo)
        d.expandedIndex = -1
        verify(!d._wiredSections[0].expanded)
        verify(!d._wiredSections[1].expanded)
    }

    function test_expandedIndex_zero_expandsFirst() {
        var d = makeWired(drawerTwo)
        d.expandedIndex = 0
        verify( d._wiredSections[0].expanded)
        verify(!d._wiredSections[1].expanded)
    }

    function test_expandedIndex_one_expandsSecond() {
        var d = makeWired(drawerTwo)
        d.expandedIndex = 1
        verify(!d._wiredSections[0].expanded)
        verify( d._wiredSections[1].expanded)
    }

    function test_expandedIndex_change_collapsesOld() {
        var d = makeWired(drawerTwo)
        d.expandedIndex = 0
        verify(d._wiredSections[0].expanded)
        d.expandedIndex = 1
        verify(!d._wiredSections[0].expanded)
        verify( d._wiredSections[1].expanded)
    }

    function test_expandedIndex_setMinusOne_collapsesPreviouslyOpen() {
        var d = makeWired(drawerTwo)
        d.expandedIndex = 0
        verify(d._wiredSections[0].expanded)
        d.expandedIndex = -1
        verify(!d._wiredSections[0].expanded)
        verify(!d._wiredSections[1].expanded)
    }

    function test_three_sections_exclusiveExpansion() {
        var d = makeWired(drawerThree)
        d.expandedIndex = 2
        verify(!d._wiredSections[0].expanded)
        verify(!d._wiredSections[1].expanded)
        verify( d._wiredSections[2].expanded)
    }

    // ── toggled signal drives expandedIndex ──────────────────────────────────
    function test_toggle_closed_section_setsExpandedIndex() {
        var d = makeWired(drawerTwo)
        d.expandedIndex = -1
        // Fire toggled on section 0 — should open it.
        d._wiredSections[0].toggled()
        compare(d.expandedIndex, 0)
    }

    function test_toggle_open_section_setsMinusOne() {
        var d = makeWired(drawerTwo)
        d.expandedIndex = 0
        // Fire toggled on the already-open section — should close all.
        d._wiredSections[0].toggled()
        compare(d.expandedIndex, -1)
    }

    function test_toggle_different_section_switchesOpen() {
        var d = makeWired(drawerTwo)
        d.expandedIndex = 0
        // Fire toggled on section 1 — section 0 must close, section 1 must open.
        d._wiredSections[1].toggled()
        compare(d.expandedIndex, 1)
    }

    // ── Direction propagation ────────────────────────────────────────────────
    function test_directionPropagated_toChildSections() {
        var d = makeWired(drawerLTR)
        compare(d._wiredSections[0].direction, AccordionDrawer.LeftToRight)
    }
}
