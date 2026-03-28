import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Courtroom screen overlay.  The game scene is rendered by SceneTextureItem
 * in Main.qml — this Item only hosts HUD elements on top of it.
 */
Item {
    anchors.fill: parent

    // Game renderer — always visible behind every screen.
    SceneTextureItem {
        anchors.fill: parent
    }

    // Top bar: character name + disconnect button.
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 48
        color: "#cc0d0d1a"

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 12

            Label {
                text: app.courtroomController.charName.length > 0 ? app.courtroomController.charName : "Courtroom"
                color: "white"
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
            }

            Label {
                text: "♪ " + app.courtroomController.nowPlaying
                color: "#aaddff"
                font.pixelSize: 13
                elide: Text.ElideRight
                visible: app.courtroomController.nowPlaying.length > 0
                Layout.preferredWidth: 200
            }

            Button {
                text: "Disconnect"
                onClicked: app.courtroomController.disconnect()
            }
        }
    }

    // OOC chat log anchored to the bottom-right.
    Rectangle {
        id: chatPanel
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: topBar.bottom
        width: Math.min(400, parent.width * 0.3)
        color: "#cc0d0d1a"

        ListView {
            anchors.fill: parent
            anchors.margins: 6
            clip: true
            model: app.courtroomController.chatModel
            verticalLayoutDirection: ListView.BottomToTop

            delegate: Label {
                width: ListView.view.width
                text: (model.isSystem ? "" : model.sender + ": ") + model.message
                color: model.isSystem ? "#aaffaa" : "white"
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }
    }
}
