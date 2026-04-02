import QtQuick
import QtQuick.Controls

/**
 * Connected player roster panel.
 * Model: controller.playerListModel
 */
Frame {
    id: root
    required property var controller
    padding: 4

    ListView {
        anchors.fill: parent
        clip: true
        model: root.controller ? root.controller.playerListModel : null

        delegate: ItemDelegate {
            width: ListView.view.width
            contentItem: Row {
                spacing: 4
                Label { text: model.name;      font.bold: true;  font.pixelSize: 11; width: 80; elide: Text.ElideRight }
                Label { text: model.character; font.pixelSize: 10; opacity: 0.75; elide: Text.ElideRight }
            }
        }

        Label {
            anchors.centerIn: parent
            text: "No players"
            visible: parent.count === 0
            opacity: 0.5
            font.pixelSize: 11
        }
    }
}
