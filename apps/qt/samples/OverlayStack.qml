import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AO.Components

/**
 * Layered OverlayPanels with different visual styles and positions.
 *
 * Demonstrates OverlayPanel's auto-sizing, background options,
 * and how multiple overlays compose on top of a base viewport.
 * The background simulates a courtroom viewport with an animated
 * gradient so the overlay transparency is clearly visible.
 */
Rectangle {
    id: root
    width: 560; height: 400
    radius: 8
    clip: true

    // ── Simulated viewport background ──────────────────────────────
    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "#1a1a2e" }
        GradientStop { position: 0.5; color: "#16213e" }
        GradientStop { position: 1.0; color: "#0f3460" }
    }

    // Animated accent line to show transparency.
    Rectangle {
        id: scanline
        width: parent.width; height: 2
        color: "#30ffffff"
        y: 0

        SequentialAnimation on y {
            loops: Animation.Infinite
            NumberAnimation { to: root.height; duration: 3000; easing.type: Easing.InOutSine }
            NumberAnimation { to: 0;           duration: 3000; easing.type: Easing.InOutSine }
        }
    }

    // Courtroom-style grid lines.
    Canvas {
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.strokeStyle = "#15ffffff";
            ctx.lineWidth = 1;
            for (var x = 0; x < width; x += 40) {
                ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke();
            }
            for (var y = 0; y < height; y += 40) {
                ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke();
            }
        }
    }

    // ── Top-left: Health badge ─────────────────────────────────────
    OverlayPanel {
        anchors { top: parent.top; left: parent.left; margins: 10 }
        backgroundColor: "#cc000000"
        backgroundRadius: 4
        padding: 8

        RowLayout {
            spacing: 8

            Rectangle {
                width: 10; height: 10; radius: 5
                color: "#4caf50"
            }

            Label {
                text: "HP: 85 / 100"
                color: "#ccffcc"
                font { pixelSize: 12; bold: true; family: "monospace" }
            }
        }
    }

    // ── Top-right: Timer ───────────────────────────────────────────
    OverlayPanel {
        anchors { top: parent.top; right: parent.right; margins: 10 }
        backgroundColor: "#cc1a1a2e"
        backgroundRadius: 4
        padding: 8

        RowLayout {
            spacing: 6

            Label {
                text: "\u23F1"
                font.pixelSize: 14
            }

            Label {
                id: timerLabel
                property int elapsed: 0
                text: {
                    var m = Math.floor(elapsed / 60);
                    var s = elapsed % 60;
                    return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s;
                }
                color: "#e0e0e0"
                font { pixelSize: 14; bold: true; family: "monospace" }

                Timer {
                    interval: 1000; running: true; repeat: true
                    onTriggered: timerLabel.elapsed++
                }
            }
        }
    }

    // ── Bottom-left: Status stack ──────────────────────────────────
    OverlayPanel {
        anchors { bottom: parent.bottom; left: parent.left; margins: 10 }
        backgroundColor: "#dd101020"
        backgroundRadius: 6
        padding: 10

        ColumnLayout {
            spacing: 4

            Label {
                text: "CONNECTION"
                color: "#666"
                font { pixelSize: 9; capitalization: Font.AllUppercase; letterSpacing: 1.5 }
            }

            RowLayout {
                spacing: 6
                Rectangle { width: 8; height: 8; radius: 4; color: "#4caf50" }
                Label { text: "Connected"; color: "#aaffaa"; font.pixelSize: 11 }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

            Label {
                text: "NETWORK"
                color: "#666"
                font { pixelSize: 9; capitalization: Font.AllUppercase; letterSpacing: 1.5 }
            }

            Repeater {
                model: [
                    { label: "Ping",    val: "42 ms",      col: "#aaffaa" },
                    { label: "Players", val: "12 online",  col: "#cccccc" },
                    { label: "Server",  val: "Vanilla",    col: "#cccccc" }
                ]
                RowLayout {
                    required property var modelData
                    spacing: 8
                    Label { text: modelData.label + ":"; color: "#777"; font.pixelSize: 10 }
                    Label { text: modelData.val; color: modelData.col; font.pixelSize: 10 }
                }
            }
        }
    }

    // ── Center: Interjection-style banner ──────────────────────────
    OverlayPanel {
        anchors.centerIn: parent
        backgroundColor: "#ee8b0000"
        backgroundRadius: 0
        padding: 0

        Rectangle {
            width: 280; height: 52
            color: "transparent"

            // Red bar with gold trim.
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.color: "#daa520"; border.width: 2
            }

            Label {
                anchors.centerIn: parent
                text: "OBJECTION!"
                color: "#ffd700"
                font { pixelSize: 22; bold: true; letterSpacing: 3 }
            }
        }
    }

    // ── Bottom-right: Mini action bar ──────────────────────────────
    OverlayPanel {
        anchors { bottom: parent.bottom; right: parent.right; margins: 10 }
        backgroundColor: "#cc000000"
        backgroundRadius: 20
        padding: 6

        Row {
            spacing: 4

            Repeater {
                model: ["\u2694", "\u2696", "\u270E", "\u2709"]

                Rectangle {
                    required property string modelData
                    width: 36; height: 36; radius: 18
                    color: toolMouse.containsMouse ? "#444" : "#2a2a2a"

                    Label {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 16
                    }

                    MouseArea {
                        id: toolMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }
        }
    }
}
