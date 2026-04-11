import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Disconnect confirmation dialog.
 * Calls app.disconnect() on confirm, which resets all courtroom controllers
 * and navigates back to the server list via UIManager.
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
                root.close()
                app.disconnect()
            }
        }
    }

    function showWithReason(msg) {
        root.reason = msg || "Connection closed."
        root.open()
    }
}
