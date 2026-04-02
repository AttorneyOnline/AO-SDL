import QtQuick
import QtQuick.Controls

/**
 * Developer debug overlay.
 * Toggle with /debug in OOC chat (wired in a future pass).
 * QtDebugContext context property will be registered alongside debug wiring.
 */
Frame {
    id: root
    padding: 8
    opacity: 0.85

    Column {
        anchors.fill: parent
        spacing: 4

        Label { text: "== Debug Overlay =="; font.bold: true; font.pixelSize: 12 }
        Label { text: "Backend: ?";          font.pixelSize: 11 }
        Label { text: "Draw calls: ?";       font.pixelSize: 11 }
        Label { text: "Internal scale: ?";   font.pixelSize: 11 }

        Row {
            spacing: 8
            Label { text: "Scale:"; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
            SpinBox {
                from: 1; to: 8; value: 4
                font.pixelSize: 11
            }
        }
    }
}
