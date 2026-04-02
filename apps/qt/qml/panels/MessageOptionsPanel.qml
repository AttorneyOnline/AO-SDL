import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * IC message options panel: flip, no-interrupt, additive, shout, effect.
 */
Frame {
    id: root
    padding: 8

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Label { text: "Message Options"; font.bold: true; font.pixelSize: 12 }

        CheckBox { text: "Flip character" }
        CheckBox { text: "No interrupt"   }
        CheckBox { text: "Additive chat"  }

        RowLayout {
            spacing: 4
            Label { text: "Shout:"; font.pixelSize: 11 }
            ComboBox {
                model: ["None", "Objection!", "Hold It!", "Take That!"]
                font.pixelSize: 11
            }
        }

        RowLayout {
            spacing: 4
            Label { text: "Effect:"; font.pixelSize: 11 }
            ComboBox {
                model: ["None", "Realization", "Whim"]
                font.pixelSize: 11
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "Close"
            onClicked: root.visible = false
        }
    }
}
