import QtQuick
import QtQuick.Controls

/**
 * Root item loaded by QtGameWindow.  Hosts a StackView that mirrors the
 * C++ UIManager screen stack.  QtGameWindow calls navigateTo(id) whenever
 * UIManager's active screen changes.
 *
 * Context properties set from C++ (see QtGameWindow::setupQml()):
 *   renderBridge          — RenderBridge*
 *   serverListController  — ServerListController*
 *   charSelectController  — CharSelectController*
 *   courtroomController   — CourtroomController*
 */
Item {
    id: root
    width:  800
    height: 600

    // Called by QtGameWindow::onScreenChanged() when the engine transitions
    // to a new screen.
    function navigateTo(screenId) {
        switch (screenId) {
        case "server_list":
            stack.replace(null, serverListPage);
            break;
        case "char_select":
            stack.push(charSelectPage);
            break;
        case "courtroom":
            stack.push(courtroomPage);
            break;
        default:
            break;
        }
    }

    StackView {
        id: stack
        anchors.fill: parent

        Component.onCompleted: stack.push(serverListPage)
    }

    Component { id: serverListPage;  ServerListScreen  {} }
    Component { id: charSelectPage;  CharSelectScreen  {} }
    Component { id: courtroomPage;   CourtroomScreen   {} }
}
