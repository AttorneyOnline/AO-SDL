import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "Attorney Online — Qt PoC"
    color: "#000"

    property int clicks: 0

    // GL triangle rendered outside the scene graph, displayed as a texture.
    SceneTextureItem {
        anchors.fill: parent
        anchors.bottomMargin: controlBar.height
    }

    // Interactive QML controls — proves input forwarding works.
    Rectangle {
        id: controlBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 80
        color: "#cc1a1a2e"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 12

            Label {
                text: "Clicks: " + root.clicks
                color: "white"
                font.pixelSize: 18
            }

            Button {
                text: "Click Me"
                onClicked: root.clicks++
            }

            Button {
                text: "Reset"
                onClicked: root.clicks = 0
            }

            TextField {
                id: input
                placeholderText: "Type here to test keyboard..."
                Layout.fillWidth: true
            }

            Label {
                text: input.text.length > 0 ? "Echo: " + input.text : ""
                color: "#88ff88"
                font.pixelSize: 14
                Layout.preferredWidth: 200
                elide: Text.ElideRight
            }
        }
    }
}
