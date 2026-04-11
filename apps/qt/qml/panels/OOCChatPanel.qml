import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * OOC chat panel: scrolling log + name field + message input.
 * controller: ChatController — provides chatModel, sendOOCMessage(name, message).
 * oocName:    Initial sender name (pre-populated from app.icController.charName).
 *             The user can override it via the name field at any time.
 */
Frame {
    id: root
    required property var controller
    property string oocName: ""
    padding: 4

    background: Rectangle {
        color: "#1e1e1e"
        border.color: "#444"
        border.width: 1
        radius: 2
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        ListView {
            id: log
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.controller ? root.controller.chatModel : null
            spacing: 1

            delegate: TextEdit {
                width: ListView.view.width
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                font.pixelSize: 11
                color: "#e0e0e0"
                // RichText from Label used inline <b>/<i> tags; keep the same format.
                text: model.isSystem
                      ? "<i style='color:#aaa'>" + model.message + "</i>"
                      : "<b style='color:#fff'>" + model.sender + ":</b> "
                        + "<span style='color:#e0e0e0'>" + model.message + "</span>"
                // Prevent the TextEdit from stealing the ListView's scroll wheel.
                Keys.forwardTo: []
            }

            onCountChanged: positionViewAtEnd()
        }

        TextField {
            id: nameInput
            Layout.fillWidth: true
            placeholderText: "OOC name…"
            font.pixelSize: 11
            text: root.oocName
            onTextEdited: root.oocName = text
            onAccepted: chatInput.forceActiveFocus()
        }

        TextField {
            id: chatInput
            Layout.fillWidth: true
            placeholderText: "OOC chat…"
            font.pixelSize: 11
            onAccepted: sendMessage()
        }
    }

    function sendMessage() {
        if (!root.controller || chatInput.text.length === 0)
            return
        root.controller.sendOOCMessage(nameInput.text, chatInput.text)
        chatInput.clear()
    }
}
