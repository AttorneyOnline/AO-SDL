import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * IC message options panel: pre/post emote, shout, effect, flip, no-interrupt.
 */
Frame {
    id: root
    padding: 8

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Label { text: "Message Options"; font.bold: true; font.pixelSize: 12 }

        CheckBox { id: flipCheck;         text: "Flip character" }
        CheckBox { id: noInterruptCheck;  text: "No interrupt"   }
        CheckBox { id: additiveChatCheck; text: "Additive chat"  }

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
