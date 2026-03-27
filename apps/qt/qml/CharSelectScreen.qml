import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: "#cc0d1a0d"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            RowLayout {
                Label {
                    text: "Character Select"
                    color: "white"
                    font.pixelSize: 24
                    font.bold: true
                    Layout.fillWidth: true
                }

                Button {
                    text: "Disconnect"
                    onClicked: app.charSelectController.disconnect()
                }
            }

            GridView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: app.charSelectController.model
                cellWidth: 120
                cellHeight: 140

                delegate: ItemDelegate {
                    width: 116
                    height: 136

                    contentItem: ColumnLayout {
                        spacing: 4

                        Rectangle {
                            width: 96
                            height: 96
                            color: model.taken ? "#44880000" : "#44004400"
                            border.color: model.taken ? "#880000" : "#008800"
                            border.width: 1
                            Layout.alignment: Qt.AlignHCenter

                            Label {
                                anchors.centerIn: parent
                                text: "?"
                                color: model.taken ? "#ff4444" : "#44ff44"
                                font.pixelSize: 32
                            }
                        }

                        Label {
                            text: model.name
                            color: model.taken ? "#888888" : "white"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }

                    onClicked: {
                        if (!model.taken)
                            app.charSelectController.selectCharacter(index)
                    }
                }
            }
        }
    }
}
