import QtQuick
import QtQuick.Controls
import AO

Item {
    id: root

    // Game viewport — renders the engine's offscreen FBO texture.
    // Visible only during courtroom (other screens overlay it fully).
    GameViewport {
        id: gameViewport
        objectName: "gameViewport"
        anchors.fill: parent
        visible: uiBridge ? uiBridge.activeScreenId === "courtroom" : false
    }

    // Screen stack — loads the QML component matching the active engine screen.
    Loader {
        id: screenLoader
        anchors.fill: parent
        source: {
            if (!uiBridge) return ""
            switch (uiBridge.activeScreenId) {
            case "server_list":
                return "screens/ServerList.qml"
            case "char_select":
                return "screens/CharSelect.qml"
            case "courtroom":
                return "screens/Courtroom.qml"
            default:
                return ""
            }
        }
    }
}
