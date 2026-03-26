import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Developer debug overlay.  Mirrors the SDL DebugOverlayWidget.
 * Toggle with /debug in OOC chat (wired in a future pass).
 *
 * Reads from QtDebugContext via context properties set from C++ (not yet
 * registered; will be added alongside the debug panel wiring).
 */
Frame {
    id: root
    padding: 8
    opacity: 0.85

    Column {
        anchors.fill: parent
        spacing: 4

        Label { text: "== Debug Overlay ==";  font.bold: true; font.pixelSize: 12 }
        Label { id: backendLabel; text: "Backend: ?";          font.pixelSize: 11 }
        Label { id: drawCallLabel; text: "Draw calls: ?";       font.pixelSize: 11 }
        Label { id: scaleLabel;    text: "Internal scale: ?";   font.pixelSize: 11 }

        Row {
            spacing: 8
            Label { text: "Scale:"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
            SpinBox {
                from: 1; to: 8; value: 4
                font.pixelSize: 11
                onValueChanged: {
                    // Will call QtDebugContext once wired.
                }
            }
        }
    }
}
