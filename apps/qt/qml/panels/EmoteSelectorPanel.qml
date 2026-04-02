import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Emote selection grid.
 * Bound to controller.emoteModel (EmoteModel: roles "comment", "iconSource").
 * Calls controller.selectEmote(index) on click.
 * Highlights the currently selected emote via controller.selectedEmote.
 */
Frame {
    id: root
    required property var controller
    padding: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Label { text: "Emotes"; font.bold: true; font.pixelSize: 12; Layout.fillWidth: true }
            CheckBox {
                id: preAnimCheck
                text: "Pre"
                font.pixelSize: 11
                checked: root.controller ? root.controller.preAnim : false
                onCheckedChanged: {
                    if (root.controller && root.controller.preAnim !== checked)
                        root.controller.preAnim = checked
                }
            }
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            cellWidth:  68
            cellHeight: 68
            clip: true
            model: root.controller ? root.controller.emoteModel : null

            delegate: ItemDelegate {
                id: cell
                width:  grid.cellWidth  - 2
                height: grid.cellHeight - 2

                required property int    index
                required property string comment
                required property string iconSource

                readonly property bool isSelected:
                    root.controller && root.controller.selectedEmote === cell.index

                background: Rectangle {
                    color: cell.isSelected ? "#5588cc"
                         : cell.hovered    ? "#555"
                                           : "#333"
                    radius: 3
                    border.color: cell.isSelected ? "#88aaff" : "transparent"
                    border.width: 1
                }

                contentItem: Item {
                    Image {
                        id: icon
                        anchors.centerIn: parent
                        width:  56
                        height: 56
                        source: cell.iconSource
                        fillMode: Image.PreserveAspectFit
                        smooth: false
                        asynchronous: true
                        visible: status === Image.Ready

                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: cell.comment
                        font.pixelSize: 9
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        width: parent.width - 4
                        visible: icon.status !== Image.Ready
                        color: "#ccc"
                    }
                }

                ToolTip.visible: hovered && cell.comment !== ""
                ToolTip.text: cell.comment
                ToolTip.delay: 500

                onClicked: {
                    if (root.controller)
                        root.controller.selectEmote(cell.index)
                }
            }
        }
    }
}
