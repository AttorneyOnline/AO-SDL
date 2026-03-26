import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Combined area / music-track list with search filter.
 * Model: courtroomController.musicAreaModel
 */
Frame {
    id: root
    padding: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            Label { text: courtroomController ? courtroomController.nowPlaying : ""; elide: Text.ElideRight; font.italic: true; font.pixelSize: 10; Layout.fillWidth: true }
        }

        TextField {
            id: filter
            Layout.fillWidth: true
            placeholderText: "Filter…"
            font.pixelSize: 11
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: courtroomController ? courtroomController.musicAreaModel : null

            delegate: ItemDelegate {
                width: ListView.view.width
                visible: !filter.text.length || model.name.toLowerCase().includes(filter.text.toLowerCase())
                height: visible ? implicitHeight : 0

                contentItem: RowLayout {
                    spacing: 4
                    Label {
                        text: model.isArea ? "▶" : "♪"
                        font.pixelSize: 10
                        opacity: 0.6
                    }
                    Label {
                        text: model.name
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        visible: model.isArea && model.playerCount >= 0
                        text: model.playerCount
                        font.pixelSize: 10
                        opacity: 0.7
                    }
                }
            }
        }
    }
}
