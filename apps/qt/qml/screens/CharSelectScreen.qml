import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Character selection screen.  Shows the character roster in a grid with
 * taken characters greyed out.  Delegates to app.charSelectController.
 */
Page {
    id: root

    header: RowLayout {
        width: parent.width
        Label { text: "Select your character"; font.bold: true; padding: 8; Layout.fillWidth: true }
        ToolButton { text: "✕ Disconnect"; onClicked: app.charSelectController.disconnect() }
    }

    GridView {
        id: grid
        anchors.fill: parent
        anchors.margins: 8
        cellWidth:  96
        cellHeight: 112
        clip: true
        cacheBuffer: 2000
        model: app.charSelectController ? app.charSelectController.model : null
        reuseItems: true

        delegate: Item {
            id: cell
            width:  grid.cellWidth  - 4
            height: grid.cellHeight - 4
            opacity: model.taken ? 0.4 : 1.0

            Rectangle {
                id: iconBg
                anchors.horizontalCenter: parent.horizontalCenter
                y: 2
                width: 64; height: 64
                color: model.taken ? "#444" : "#666"
                radius: 4

                Image {
                    id: iconImg
                    anchors.fill: parent
                    source: model.iconSource
                    sourceSize: Qt.size(64, 64)
                    fillMode: Image.PreserveAspectFit
                    visible: status === Image.Ready
                    cache: true
                }

                Label {
                    anchors.centerIn: parent
                    text: model.name.charAt(0).toUpperCase()
                    color: "white"
                    visible: !iconImg.visible
                }
            }

            Label {
                anchors.top: iconBg.bottom
                anchors.topMargin: 2
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 4
                text: model.name
                font.pixelSize: 10
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }

            MouseArea {
                anchors.fill: parent
                onClicked: app.charSelectController.selectCharacter(index)
            }
        }
    }
}
