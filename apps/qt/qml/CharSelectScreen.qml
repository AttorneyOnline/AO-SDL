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
                id: grid
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                cacheBuffer: 2000
                model: app.charSelectController.model
                cellWidth: 120
                cellHeight: 140
                reuseItems: true

                delegate: Item {
                    id: cell
                    width: 116
                    height: 136

                    Rectangle {
                        id: iconBg
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 4
                        width: 96; height: 96
                        color: model.taken ? "#44880000" : "#44004400"
                        border.color: model.taken ? "#880000" : "#008800"
                        border.width: 1

                        Image {
                            id: iconImage
                            anchors.fill: parent
                            source: model.iconSource
                            sourceSize: Qt.size(96, 96)
                            fillMode: Image.PreserveAspectFit
                            visible: status === Image.Ready
                            cache: true
                        }

                        Label {
                            anchors.centerIn: parent
                            text: "?"
                            color: model.taken ? "#ff4444" : "#44ff44"
                            font.pixelSize: 32
                            visible: !iconImage.visible
                        }
                    }

                    Label {
                        anchors.top: iconBg.bottom
                        anchors.topMargin: 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: parent.width - 4
                        text: model.name
                        color: model.taken ? "#888888" : "white"
                        font.pixelSize: 10
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (!model.taken)
                                app.charSelectController.selectCharacter(index)
                        }
                    }
                }
            }
        }
    }
}
