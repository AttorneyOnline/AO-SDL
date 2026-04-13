import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Stat-card dashboard with animated progress bars.
 *
 * Demonstrates GridLayout, component reuse via inline components,
 * gradient fills, and NumberAnimation.  Each card shows a label,
 * a value, and a progress bar that animates to its target on load.
 */
Rectangle {
    id: root
    width: 480; height: 360
    color: "#121218"
    radius: 8

    component StatCard: Rectangle {
        id: card
        required property string label
        required property int    value
        required property int    maxValue
        required property color  accent

        radius: 6
        color: "#1c1c24"
        border.color: "#2a2a35"
        border.width: 1

        implicitHeight: 110

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 6

            Label {
                text: card.label
                color: "#777"
                font { pixelSize: 11; capitalization: Font.AllUppercase; letterSpacing: 1.2 }
            }

            Label {
                text: card.value.toLocaleString()
                color: "#eee"
                font { pixelSize: 28; bold: true }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Track
                Rectangle {
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                    height: 6; radius: 3
                    color: "#2a2a35"

                    // Fill
                    Rectangle {
                        id: fill
                        height: parent.height; radius: 3
                        width: 0
                        color: card.accent

                        NumberAnimation on width {
                            to: fill.parent.width * (card.value / card.maxValue)
                            duration: 800
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }

            Label {
                text: Math.round(card.value / card.maxValue * 100) + "% of target"
                color: "#555"
                font.pixelSize: 10
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "Server Dashboard"
            color: "#ccc"
            font { pixelSize: 16; bold: true }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            StatCard {
                Layout.fillWidth: true; Layout.fillHeight: true
                label: "Players Online"; value: 847; maxValue: 1200
                accent: "#4fc3f7"
            }
            StatCard {
                Layout.fillWidth: true; Layout.fillHeight: true
                label: "Active Cases"; value: 23; maxValue: 50
                accent: "#81c784"
            }
            StatCard {
                Layout.fillWidth: true; Layout.fillHeight: true
                label: "Messages / hr"; value: 3842; maxValue: 5000
                accent: "#ffb74d"
            }
            StatCard {
                Layout.fillWidth: true; Layout.fillHeight: true
                label: "Uptime (hrs)"; value: 718; maxValue: 744
                accent: "#ce93d8"
            }
        }
    }
}
