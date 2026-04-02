import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Pure display component: Defence / Prosecution health bars.
 * All state is driven by properties — no controller dependency.
 */
Item {
    id: root
    width: 130
    height: 48

    property int defHp: 0
    property int proHp: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            spacing: 4
            Label { text: "DEF"; font.pixelSize: 10; font.bold: true; color: "#4af" }
            ProgressBar {
                from: 0; to: 10
                value: root.defHp
                Layout.fillWidth: true
            }
        }
        RowLayout {
            spacing: 4
            Label { text: "PRO"; font.pixelSize: 10; font.bold: true; color: "#f44" }
            ProgressBar {
                from: 0; to: 10
                value: root.proHp
                Layout.fillWidth: true
            }
        }
    }
}
