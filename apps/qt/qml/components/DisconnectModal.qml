import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Disconnect confirmation dialog.  Shown when the server closes the
 * connection (DisconnectEvent) or the user clicks the disconnect button.
 *
 * Calls app.courtroomController.disconnect() on confirm, which emits
 * POP_TO_ROOT → QtGameWindow::onNavAction().
 */
Dialog {
    id: root
    title: "Disconnected"
    modal: true
    standardButtons: Dialog.NoButton
    anchors.centerIn: parent
    width: 280

    property string reason: "Connection closed."

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: root.reason
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "Return to Server List"
            onClicked: {
                root.close();
                app.courtroomController.disconnect();
            }
        }
    }

    /// Show the modal with an optional reason string.
    function showWithReason(msg) {
        root.reason = msg || "Connection closed.";
        root.open();
    }
}
