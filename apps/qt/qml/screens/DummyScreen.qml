import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Placeholder screen used during navigation tests.
 */
Item {
    id: root
    anchors.fill: parent

    property var controller: app.dummyController

    Rectangle {
        anchors.fill: parent
        color: "#cc1a0d0d"

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20

            Label {
                text: "Dummy Screen"
                color: "white"
                font.pixelSize: 32
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Navigation works. RGB cycling is still running behind this screen."
                color: "#cccccc"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
            }

            Button {
                text: "Go Back"
                Layout.alignment: Qt.AlignHCenter
                onClicked: root.controller.goBack()
            }
        }
    }
}
