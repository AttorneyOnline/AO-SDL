import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "Attorney Online"
    color: "#000"

    // Game renderer — always visible behind every screen.
    SceneTextureItem {
        anchors.fill: parent
    }

    // Navigation: swap QML screens by mapping UIManager's active screen id.
    Loader {
        anchors.fill: parent
        source: {
            switch (app.currentScreenId) {
                case "server_list": return "ServerListScreen.qml"
                case "char_select": return "CharSelectScreen.qml"
                case "courtroom":   return "CourtroomScreen.qml"
                case "dummy":       return "DummyScreen.qml"
                default:            return ""
            }
        }
    }
}
