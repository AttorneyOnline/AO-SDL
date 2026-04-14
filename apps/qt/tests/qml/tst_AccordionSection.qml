import QtQuick
import QtTest
import AO.Components

// Tests for AccordionSection
//
// Covered:
//   - Default property values
//   - Direction enum integer values (TopToBottom=0 … RightToLeft=3)
//   - _isHorizontal derived flag for all four directions
//   - _headerSize constant (28 px)
//   - _targetSize = _headerSize + (expanded ? _contentSize : 0)
//   - _arrowChar selection based on direction × expanded state
//   - toggled signal fires when the header bar is clicked
//   - expanded can be set programmatically

TestCase {
    name: "AccordionSection"
    when: windowShown

    // ── Components ───────────────────────────────────────────────────────────
    Component {
        id: plainSection
        AccordionSection {}
    }

    Component {
        id: noAnim
        // animationDuration: 0 so size changes are instant
        AccordionSection { animationDuration: 0 }
    }

    Component {
        id: ltrSection
        AccordionSection { direction: AccordionSection.LeftToRight; animationDuration: 0 }
    }
    Component {
        id: rtlSection
        AccordionSection { direction: AccordionSection.RightToLeft; animationDuration: 0 }
    }
    Component {
        id: ttbSection
        AccordionSection { direction: AccordionSection.TopToBottom; animationDuration: 0 }
    }
    Component {
        id: bttSection
        AccordionSection { direction: AccordionSection.BottomToTop; animationDuration: 0 }
    }

    Component {
        id: clickableSection
        AccordionSection {
            width: 200
            height: 200
            title: "Click me"
            animationDuration: 0
        }
    }

    // ── SignalSpy for toggled ────────────────────────────────────────────────
    SignalSpy {
        id: toggledSpy
        signalName: "toggled"
    }

    // ── Default properties ───────────────────────────────────────────────────
    function test_defaults() {
        var s = createTemporaryObject(plainSection, this)
        verify(s !== null, "object created")
        compare(s.title,             "")
        compare(s.expanded,          false)
        compare(s.animationDuration, 200)
        compare(s.direction,         AccordionSection.TopToBottom)
    }

    // ── Direction enum values ────────────────────────────────────────────────
    function test_enum_TopToBottom() { compare(AccordionSection.TopToBottom, 0) }
    function test_enum_BottomToTop() { compare(AccordionSection.BottomToTop, 1) }
    function test_enum_LeftToRight() { compare(AccordionSection.LeftToRight, 2) }
    function test_enum_RightToLeft() { compare(AccordionSection.RightToLeft, 3) }

    // ── _isHorizontal ────────────────────────────────────────────────────────
    function test_isHorizontal_TopToBottom_isFalse() {
        var s = createTemporaryObject(ttbSection, this)
        verify(!s._isHorizontal)
    }
    function test_isHorizontal_BottomToTop_isFalse() {
        var s = createTemporaryObject(bttSection, this)
        verify(!s._isHorizontal)
    }
    function test_isHorizontal_LeftToRight_isTrue() {
        var s = createTemporaryObject(ltrSection, this)
        verify(s._isHorizontal)
    }
    function test_isHorizontal_RightToLeft_isTrue() {
        var s = createTemporaryObject(rtlSection, this)
        verify(s._isHorizontal)
    }

    // ── direction changes _isHorizontal ──────────────────────────────────────
    function test_direction_change_updatesIsHorizontal() {
        var s = createTemporaryObject(ttbSection, this)
        verify(!s._isHorizontal)
        s.direction = AccordionSection.LeftToRight
        verify(s._isHorizontal)
        s.direction = AccordionSection.TopToBottom
        verify(!s._isHorizontal)
    }

    // ── _headerSize constant ─────────────────────────────────────────────────
    function test_headerSize_is28() {
        var s = createTemporaryObject(plainSection, this)
        compare(s._headerSize, 28)
    }

    // ── _targetSize ──────────────────────────────────────────────────────────
    function test_targetSize_collapsed_equalsHeaderSize() {
        var s = createTemporaryObject(noAnim, this)
        // No content ⇒ _contentSize = 0; collapsed ⇒ _targetSize = headerSize.
        compare(s._targetSize, s._headerSize)
    }

    function test_targetSize_expanded_equalsHeaderPlusContent() {
        var s = createTemporaryObject(noAnim, this)
        s.expanded = true
        compare(s._targetSize, s._headerSize + s._contentSize)
    }

    function test_targetSize_collapseAfterExpand_returnsToHeaderSize() {
        var s = createTemporaryObject(noAnim, this)
        s.expanded = true
        s.expanded = false
        compare(s._targetSize, s._headerSize)
    }

    // ── _arrowChar per direction × expanded ─────────────────────────────────
    function test_arrowChar_TopToBottom_collapsed() {
        var s = createTemporaryObject(ttbSection, this)
        compare(s._arrowChar, "\u25B8")   // ▸ closed
    }
    function test_arrowChar_TopToBottom_expanded() {
        var s = createTemporaryObject(ttbSection, this)
        s.expanded = true
        compare(s._arrowChar, "\u25BE")   // ▾ open
    }
    function test_arrowChar_BottomToTop_collapsed() {
        var s = createTemporaryObject(bttSection, this)
        compare(s._arrowChar, "\u25B8")   // ▸ closed
    }
    function test_arrowChar_BottomToTop_expanded() {
        var s = createTemporaryObject(bttSection, this)
        s.expanded = true
        compare(s._arrowChar, "\u25B4")   // ▴ open
    }
    function test_arrowChar_LeftToRight_collapsed() {
        var s = createTemporaryObject(ltrSection, this)
        compare(s._arrowChar, "\u25BE")   // ▾ closed
    }
    function test_arrowChar_LeftToRight_expanded() {
        var s = createTemporaryObject(ltrSection, this)
        s.expanded = true
        compare(s._arrowChar, "\u25B8")   // ▸ open
    }
    function test_arrowChar_RightToLeft_collapsed() {
        var s = createTemporaryObject(rtlSection, this)
        compare(s._arrowChar, "\u25BE")   // ▾ closed
    }
    function test_arrowChar_RightToLeft_expanded() {
        var s = createTemporaryObject(rtlSection, this)
        s.expanded = true
        compare(s._arrowChar, "\u25C2")   // ◂ open
    }

    // ── toggled signal ───────────────────────────────────────────────────────
    function test_headerClick_emitsToggled() {
        var s = createTemporaryObject(clickableSection, this)
        toggledSpy.target = s
        toggledSpy.clear()
        // Click in the middle of the 28 px header strip.
        mouseClick(s, s.width / 2, s._headerSize / 2)
        compare(toggledSpy.count, 1)
    }

    function test_headerClick_twice_emitsToggledTwice() {
        var s = createTemporaryObject(clickableSection, this)
        toggledSpy.target = s
        toggledSpy.clear()
        mouseClick(s, s.width / 2, s._headerSize / 2)
        mouseClick(s, s.width / 2, s._headerSize / 2)
        compare(toggledSpy.count, 2)
    }

    // ── expanded property ────────────────────────────────────────────────────
    function test_expanded_programmaticSet() {
        var s = createTemporaryObject(noAnim, this)
        compare(s.expanded, false)
        s.expanded = true
        compare(s.expanded, true)
        s.expanded = false
        compare(s.expanded, false)
    }

    // ── title property ───────────────────────────────────────────────────────
    function test_title_isStored() {
        var s = createTemporaryObject(clickableSection, this)
        compare(s.title, "Click me")
    }
}
