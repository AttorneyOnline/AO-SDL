import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    anchors.fill: parent

    // Semi-transparent panel so the RGB cycling background is still visible.
    Rectangle {
        anchors.fill: parent
        color: "#cc0d0d1a"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Label {
                text: "Server List"
                color: "white"
                font.pixelSize: 24
                font.bold: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: app.serverListController.model

                delegate: ItemDelegate {
                    width: ListView.view.width
                    contentItem: Label {
                        text: model.name + " — " + model.hostname
                        color: "white"
                        font.pixelSize: 14
                        elide: Text.ElideRight
                    }
                    onClicked: app.serverListController.connectToServer(index)
                }
            }

            RowLayout {
                spacing: 12

                TextField {
                    id: hostField
                    placeholderText: "host:port"
                    Layout.fillWidth: true
                }

                Button {
                    text: "Direct Connect"
                    enabled: hostField.text.length > 0
                    onClicked: {
                        var parts = hostField.text.split(":")
                        var host  = parts[0]
                        var port  = parts.length > 1 ? parseInt(parts[1]) : 27016
                        app.serverListController.directConnect(host, port)
                    }
                }
            }
        }
    }
}
