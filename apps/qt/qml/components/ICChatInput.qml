import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * IC message composition panel: character name, text field, emote/side
 * selectors, and send button.
 */
Frame {
    id: root
    padding: 4

    RowLayout {
        anchors.fill: parent
        spacing: 4

        // Character name badge
        Label {
            text: app.courtroomController ? app.courtroomController.charName : ""
            font.bold: true
            font.pixelSize: 11
            elide: Text.ElideRight
            Layout.preferredWidth: 80
        }

        // Message input
        TextField {
            id: msgField
            Layout.fillWidth: true
            Layout.fillHeight: true
            placeholderText: "IC message…"
            font.pixelSize: 11
            wrapMode: TextInput.Wrap
        }

        // Toolbar: emote, side, options, send
        ColumnLayout {
            spacing: 2
            Button { text: "Emote";   font.pixelSize: 10; implicitHeight: 22 }
            Button { text: "Side";    font.pixelSize: 10; implicitHeight: 22 }
            Button { text: "Options"; font.pixelSize: 10; implicitHeight: 22 }
            Button {
                text: "Send"
                font.pixelSize: 10
                implicitHeight: 22
                onClicked: msgField.clear()
            }
        }
    }
}
