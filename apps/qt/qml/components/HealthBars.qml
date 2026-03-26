import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Defense / Prosecution health bars.
 * Reads defHp (0–10) and proHp (0–10) from courtroomController.
 */
Item {
    id: root
    width: 130
    height: 48

    readonly property int defHp: courtroomController ? courtroomController.defHp : 0
    readonly property int proHp: courtroomController ? courtroomController.proHp : 0

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
