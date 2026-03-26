import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Character selection screen.  Shows the character roster in a grid with
 * taken characters greyed out.  Delegates to charSelectController.
 */
Page {
    id: root

    header: RowLayout {
        width: parent.width
        Label { text: "Select your character"; font.bold: true; padding: 8; Layout.fillWidth: true }
        ToolButton { text: "✕ Disconnect"; onClicked: charSelectController.disconnect() }
    }

    GridView {
        id: grid
        anchors.fill: parent
        anchors.margins: 8
        cellWidth:  96
        cellHeight: 112
        clip: true
        model: charSelectController ? charSelectController.model : null

        delegate: ItemDelegate {
            width:  grid.cellWidth  - 4
            height: grid.cellHeight - 4
            opacity: model.taken ? 0.4 : 1.0
            highlighted: GridView.isCurrentItem

            contentItem: ColumnLayout {
                spacing: 2
                // Icon placeholder — real icons come via a Qt image provider in a later pass.
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 64; height: 64
                    color: model.taken ? "#444" : "#666"
                    radius: 4
                    Label { anchors.centerIn: parent; text: model.name.charAt(0).toUpperCase(); color: "white" }
                }
                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: model.name
                    font.pixelSize: 10
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            onClicked: charSelectController.selectCharacter(index)
        }
    }
}
