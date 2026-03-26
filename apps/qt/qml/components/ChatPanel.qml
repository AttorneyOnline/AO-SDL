import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * OOC chat panel: scrolling log + text input.
 * Model: courtroomController.chatModel
 */
Frame {
    id: root
    padding: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        ListView {
            id: log
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: courtroomController ? courtroomController.chatModel : null
            verticalLayoutDirection: ListView.BottomToTop

            delegate: Label {
                width: ListView.view.width
                text: (model.isSystem ? "<i>" : "<b>" + model.sender + ":</b> ") + model.message
                textFormat: Text.RichText
                wrapMode: Text.Wrap
                font.pixelSize: 11
            }

            onCountChanged: positionViewAtEnd()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            TextField {
                id: chatInput
                Layout.fillWidth: true
                placeholderText: "OOC chat…"
                font.pixelSize: 11
                onAccepted: {
                    // OutgoingChatEvent is published via the engine; placeholder.
                    chatInput.clear();
                }
            }
        }
    }
}
