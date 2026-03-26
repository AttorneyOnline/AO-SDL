import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Evidence list panel.  Model: courtroomController.evidenceModel
 */
Frame {
    id: root
    padding: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Label { text: "Evidence"; font.bold: true; font.pixelSize: 12 }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: courtroomController ? courtroomController.evidenceModel : null

            delegate: ItemDelegate {
                width: ListView.view.width
                contentItem: ColumnLayout {
                    spacing: 1
                    Label { text: model.name;        font.bold: true;  font.pixelSize: 11 }
                    Label { text: model.description; wrapMode: Text.Wrap; font.pixelSize: 10; opacity: 0.8 }
                }
            }

            Label {
                anchors.centerIn: parent
                text: "No evidence"
                visible: parent.count === 0
                opacity: 0.5
            }
        }
    }
}
