import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Interactive HSL color picker.
 *
 * Demonstrates Canvas painting, slider controls, and live property
 * bindings.  The hue wheel is drawn procedurally; saturation and
 * lightness are controlled with sliders.  The selected colour is
 * shown as a large swatch with its hex code.
 */
Rectangle {
    id: root
    width: 360; height: 420
    color: "#1e1e1e"
    radius: 8

    property real hue: hueSlider.value
    property real sat: satSlider.value
    property real lit: litSlider.value

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // ── Hue wheel ──────────────────────────────────────────────
        Canvas {
            id: hueWheel
            Layout.fillWidth: true
            Layout.preferredHeight: 160

            onPaint: {
                var ctx = getContext("2d");
                var w = width, h = height;
                ctx.clearRect(0, 0, w, h);

                var barH = h - 20;
                for (var x = 0; x < w; x++) {
                    var hueVal = x / w;
                    ctx.fillStyle = Qt.hsla(hueVal, root.sat, root.lit, 1.0);
                    ctx.fillRect(x, 0, 1, barH);
                }

                // Position indicator
                var markerX = root.hue * w;
                ctx.strokeStyle = "white";
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.moveTo(markerX, 0);
                ctx.lineTo(markerX, barH);
                ctx.stroke();

                // Gradient label
                ctx.fillStyle = "#888";
                ctx.font = "10px sans-serif";
                ctx.fillText("0\u00B0", 2, barH + 14);
                ctx.fillText("360\u00B0", w - 26, barH + 14);
            }

            // Repaint whenever any parameter changes.
            Component.onCompleted: requestPaint()
            Connections {
                target: root
                function onHueChanged() { hueWheel.requestPaint() }
                function onSatChanged() { hueWheel.requestPaint() }
                function onLitChanged() { hueWheel.requestPaint() }
            }
        }

        // ── Sliders ────────────────────────────────────────────────
        GridLayout {
            columns: 2
            columnSpacing: 8
            rowSpacing: 4
            Layout.fillWidth: true

            Label { text: "Hue"; color: "#aaa"; font.pixelSize: 11 }
            Slider {
                id: hueSlider
                Layout.fillWidth: true
                from: 0; to: 1; value: 0.55
            }

            Label { text: "Saturation"; color: "#aaa"; font.pixelSize: 11 }
            Slider {
                id: satSlider
                Layout.fillWidth: true
                from: 0; to: 1; value: 0.72
            }

            Label { text: "Lightness"; color: "#aaa"; font.pixelSize: 11 }
            Slider {
                id: litSlider
                Layout.fillWidth: true
                from: 0; to: 1; value: 0.52
            }
        }

        // ── Colour swatch ──────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 6
            color: Qt.hsla(root.hue, root.sat, root.lit, 1.0)

            Label {
                anchors.centerIn: parent
                text: parent.color.toString().toUpperCase()
                color: root.lit > 0.55 ? "#000000" : "#ffffff"
                font { pixelSize: 18; bold: true; family: "monospace" }
            }
        }
    }
}
