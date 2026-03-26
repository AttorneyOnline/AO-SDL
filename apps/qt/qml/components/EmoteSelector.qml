import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Emote selection grid.  Populated from CourtroomScreen character data
 * in a future pass; placeholder grid until emote assets are wired up.
 */
Frame {
    id: root
    padding: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Label { text: "Emotes"; font.bold: true; font.pixelSize: 12 }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            cellWidth:  64
            cellHeight: 64
            clip: true
            model: 0  // filled by CourtroomController once emote icons are loaded

            delegate: ItemDelegate {
                width:  grid.cellWidth  - 2
                height: grid.cellHeight - 2
                Rectangle {
                    anchors.fill: parent
                    color: parent.hovered ? "#555" : "#444"
                    radius: 3
                    Label { anchors.centerIn: parent; text: index; font.pixelSize: 10 }
                }
            }
        }
    }
}
