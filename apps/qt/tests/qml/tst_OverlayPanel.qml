import QtQuick
import QtTest
import AO.Components

// Tests for OverlayPanel
//
// Covered:
//   - Default property values (padding, backgroundRadius, backgroundColor)
//   - implicitWidth/implicitHeight with no content is 2 × padding
//   - backgroundColor.a == 0 for the default transparent colour
//   - backgroundColor.a  > 0 when a semi-opaque colour is set
//   - backgroundRadius is stored and readable
//   - padding and backgroundColor can be changed after creation

TestCase {
    name: "OverlayPanel"

    // ── Components ───────────────────────────────────────────────────────────
    Component {
        id: plainPanel
        OverlayPanel {}
    }

    Component {
        id: paddedPanel
        OverlayPanel { padding: 10 }
    }

    Component {
        id: coloredPanel
        OverlayPanel {
            backgroundColor: "#cc000000"
            backgroundRadius: 6
        }
    }

    Component {
        id: semiTransparentPanel
        OverlayPanel { backgroundColor: "#7f123456" }
    }

    // ── Default values ───────────────────────────────────────────────────────
    function test_defaults() {
        var p = createTemporaryObject(plainPanel, this)
        verify(p !== null, "object created")
        compare(p.padding,          0)
        compare(p.backgroundRadius, 0)
        compare(p.backgroundColor,  Qt.color("transparent"))
    }

    // ── Implicit sizing without content ──────────────────────────────────────
    function test_implicitSize_noContent_noPadding() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p.implicitWidth,  0)
        compare(p.implicitHeight, 0)
    }

    function test_implicitSize_noContent_withPadding() {
        var p = createTemporaryObject(paddedPanel, this)
        compare(p.implicitWidth,  20)   // 2 × padding(10)
        compare(p.implicitHeight, 20)
    }

    function test_implicitSize_reflects_padding_change() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p.implicitWidth,  0)
        p.padding = 5
        compare(p.implicitWidth,  10)   // 2 × 5
        compare(p.implicitHeight, 10)
    }

    // ── Background colour ────────────────────────────────────────────────────
    function test_backgroundColor_defaultIsTransparent() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p.backgroundColor.a, 0.0)
    }

    function test_backgroundColor_opaqueColorHasPositiveAlpha() {
        var p = createTemporaryObject(coloredPanel, this)
        verify(p.backgroundColor.a > 0, "alpha > 0 for non-transparent colour")
    }

    function test_backgroundColor_semiTransparentHasPositiveAlpha() {
        var p = createTemporaryObject(semiTransparentPanel, this)
        verify(p.backgroundColor.a > 0)
    }

    function test_backgroundColor_canBeSetAfterCreation() {
        var p = createTemporaryObject(plainPanel, this)
        compare(p.backgroundColor.a, 0.0)
        p.backgroundColor = "#ff0000ff"
        verify(p.backgroundColor.a > 0)
    }

    // ── backgroundRadius ─────────────────────────────────────────────────────
    function test_backgroundRadius_isStored() {
        var p = createTemporaryObject(coloredPanel, this)
        compare(p.backgroundRadius, 6)
    }

    function test_backgroundRadius_canBeChanged() {
        var p = createTemporaryObject(plainPanel, this)
        p.backgroundRadius = 12
        compare(p.backgroundRadius, 12)
    }

    // ── padding ──────────────────────────────────────────────────────────────
    function test_padding_isStored() {
        var p = createTemporaryObject(paddedPanel, this)
        compare(p.padding, 10)
    }
}
