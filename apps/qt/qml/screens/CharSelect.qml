import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    readonly property int iconSize: 64
    readonly property int gridSpacing: 4

    // Incremented to bust the image provider cache when icons load.
    property int iconGeneration: 0

    Connections {
        target: uiBridge ? uiBridge.charListModel : null
        function onDataChanged() { root.iconGeneration++ }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Character grid
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1a1a2e"
            radius: 4
            clip: true

            Label {
                anchors.centerIn: parent
                text: "Waiting for character list..."
                color: "#888"
                font.pixelSize: 16
                visible: !charGrid.model || charGrid.count === 0
            }

            GridView {
                id: charGrid
                anchors.fill: parent
                anchors.margins: 6
                cellWidth: root.iconSize + root.gridSpacing
                cellHeight: root.iconSize + root.gridSpacing
                model: uiBridge ? uiBridge.charListModel : null
                currentIndex: uiBridge ? uiBridge.selectedChar : -1

                delegate: Rectangle {
                    id: charDelegate
                    required property int index
                    required property string folder
                    required property bool hasIcon
                    required property bool taken

                    width: root.iconSize
                    height: root.iconSize
                    radius: 3

                    color: {
                        if (charGrid.currentIndex === index)
                            return "#4a4a7e"
                        if (taken)
                            return "#2a1a1a"
                        return mouseArea.containsMouse ? "#3a3a5e" : "#24243a"
                    }

                    // Character icon from the engine's decoded pixel data
                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        fillMode: Image.PreserveAspectFit
                        visible: charDelegate.hasIcon
                        cache: false
                        source: charDelegate.hasIcon
                            ? "image://charicon/" + charDelegate.index + "?g=" + root.iconGeneration
                            : ""
                    }

                    // Folder name fallback when icon hasn't loaded yet
                    Label {
                        anchors.centerIn: parent
                        width: parent.width - 4
                        visible: !charDelegate.hasIcon
                        text: charDelegate.folder
                        color: charDelegate.taken ? "#666" : "#aaa"
                        font.pixelSize: 9
                        wrapMode: Text.WrapAnywhere
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        maximumLineCount: 3
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (uiBridge)
                                uiBridge.select_character(charDelegate.index)
                        }

                        ToolTip.visible: containsMouse
                        ToolTip.delay: 400
                        ToolTip.text: charDelegate.folder + (charDelegate.taken ? " (taken)" : "")
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    active: true
                }
            }
        }

        // Bottom bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: uiBridge ? uiBridge.charCount + " characters" : ""
                color: "#888"
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Disconnect"
                onClicked: {
                    if (uiBridge) uiBridge.request_disconnect()
                }
            }
        }
    }
}
