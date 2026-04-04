import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "Attorney Online"
    color: "#000"

    // Navigation: swap screens by mapping UIManager's active screen id.
    Loader {
        anchors.fill: parent
        source: {
            switch (app.currentScreenId) {
            case "server_list": return "screens/ServerListScreen.qml"
            case "char_select": return "screens/CharSelectScreen.qml"
            case "courtroom":   return "screens/CourtroomScreen.qml"
            default:            return ""
            }
        }
    }

    // Debug overlay — toggled with F12.
    DebugOverlay {
        id: debugOverlay
        visible: false
    }

    Shortcut {
        sequence: "F12"
        onActivated: debugOverlay.visible = !debugOverlay.visible
    }
}
