import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Connected player roster panel.
 * Model: controller.playerListModel
 * Character icons are loaded via the charicon image provider (image://charicon/<folder>).
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

            contentItem: RowLayout {
                spacing: 6

                Rectangle {
                    width: 32; height: 32
                    color: "#555"
                    radius: 3
                    Layout.alignment: Qt.AlignVCenter

                    Image {
                        anchors.fill: parent
                        source: model.character ? "image://charicon/" + model.character : ""
                        sourceSize: Qt.size(32, 32)
                        fillMode: Image.PreserveAspectFit
                        visible: status === Image.Ready
                        cache: true
                    }
                }

                ColumnLayout {
                    spacing: 1
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        text: model.name
                        font.bold: true
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Label {
                        text: model.character
                        font.pixelSize: 10
                        opacity: 0.75
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
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
