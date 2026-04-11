import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * IC message options panel: flip, no-interrupt, additive, shout, effect.
 * controller: ICController
 */
Frame {
    id: root
    required property var controller
    padding: 8

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Label { text: "Message Options"; font.bold: true; font.pixelSize: 12 }

        CheckBox {
            text: "Flip character"
            checked: root.controller ? root.controller.flip : false
            onCheckedChanged: { if (root.controller && root.controller.flip !== checked) root.controller.flip = checked }
        }
        CheckBox {
            text: "No interrupt"
            checked: root.controller ? root.controller.noInterrupt : false
            onCheckedChanged: { if (root.controller && root.controller.noInterrupt !== checked) root.controller.noInterrupt = checked }
        }
        CheckBox {
            text: "Additive chat"
            checked: root.controller ? root.controller.additive : false
            onCheckedChanged: { if (root.controller && root.controller.additive !== checked) root.controller.additive = checked }
        }

        RowLayout {
            spacing: 4
            Label { text: "Shout:"; font.pixelSize: 11 }
            ComboBox {
                model: ["None", "Objection!", "Hold It!", "Take That!"]
                font.pixelSize: 11
                currentIndex: root.controller ? root.controller.objectionMod : 0
                onActivated: { if (root.controller) root.controller.objectionMod = currentIndex }
            }
        }

        RowLayout {
            spacing: 4
            Label { text: "Effect:"; font.pixelSize: 11 }
            ComboBox {
                model: ["None", "Realization", "Whim"]
                font.pixelSize: 11
                currentIndex: root.controller ? root.controller.effectMod : 0
                onActivated: { if (root.controller) root.controller.effectMod = currentIndex }
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "Close"
            onClicked: root.visible = false
        }
    }
}
