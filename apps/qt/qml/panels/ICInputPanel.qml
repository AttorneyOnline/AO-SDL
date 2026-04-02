import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * IC message composition panel.
 * Emits signals for overlay requests so the parent Screen controls visibility.
 * Calls controller.sendICMessage(text, objectionMod) to publish outgoing IC events.
 */
Frame {
    id: root
    required property var controller
    padding: 4

    signal emoteRequested()
    signal sideRequested()
    signal optionsRequested()

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: root.controller ? root.controller.charName : ""
                font.bold: true
                font.pixelSize: 11
                elide: Text.ElideRight
                Layout.preferredWidth: 80
            }

            TextField {
                id: shownameField
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: 11
                placeholderText: "Showname…"
                text: root.controller ? root.controller.showname : ""
                onEditingFinished: {
                    if (root.controller)
                        root.controller.showname = text
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            TextField {
                id: msgField
                Layout.fillWidth: true
                Layout.fillHeight: true
                placeholderText: "IC message…"
                font.pixelSize: 11
                wrapMode: TextInput.Wrap

                Keys.onReturnPressed: sendMessage()
                Keys.onEnterPressed:  sendMessage()
            }

            ColumnLayout {
                spacing: 2
                Button { text: "Emote";   font.pixelSize: 10; implicitHeight: 22; onClicked: root.emoteRequested()   }
                Button { text: "Side";    font.pixelSize: 10; implicitHeight: 22; onClicked: root.sideRequested()    }
                Button { text: "Options"; font.pixelSize: 10; implicitHeight: 22; onClicked: root.optionsRequested() }
                Button {
                    text: "Send"
                    font.pixelSize: 10
                    implicitHeight: 22
                    onClicked: sendMessage()
                }
            }
        }
    }

    function sendMessage() {
        if (!root.controller || msgField.text.length === 0)
            return
        root.controller.sendICMessage(msgField.text)
        msgField.clear()
    }
}
