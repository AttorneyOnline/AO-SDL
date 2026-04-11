import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Evidence list panel.
 * controller: EvidenceController — evidenceModel, selectedIndex, selectEvidence(i).
 *
 * Clicking an entry selects it (highlighted). A second click on the same
 * entry deselects it (-1).  The selected evidence index is read by ICController
 * when composing an IC message.
 */
Frame {
    id: root
    required property var controller
    padding: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Label { text: "Evidence"; font.bold: true; font.pixelSize: 12 }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.controller ? root.controller.evidenceModel : null

            delegate: ItemDelegate {
                id: cell
                width: ListView.view.width

                required property int index
                readonly property bool isSelected:
                    root.controller && root.controller.selectedIndex === cell.index

                highlighted: isSelected

                contentItem: ColumnLayout {
                    spacing: 1
                    Label { text: model.name;        font.bold: true; font.pixelSize: 11 }
                    Label { text: model.description; wrapMode: Text.Wrap; font.pixelSize: 10; opacity: 0.8 }
                }

                onClicked: {
                    if (!root.controller) return
                    // Toggle: clicking the selected item deselects it.
                    root.controller.selectEvidence(cell.isSelected ? -1 : cell.index)
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
