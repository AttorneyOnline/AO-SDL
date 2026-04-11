import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Side selection panel: Defence, Prosecution, Witness, Judge.
 * controller: ICController — reads side, calls selectSide(id).
 */
Frame {
    id: root
    required property var controller
    padding: 4

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Label {
            text: "Choose your side"
            font.bold: true
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter
        }

        GridLayout {
            columns: 2
            Layout.fillWidth: true
            columnSpacing: 6
            rowSpacing: 6

            Repeater {
                model: [
                    { id: "def", label: "Defence"     },
                    { id: "pro", label: "Prosecution" },
                    { id: "wit", label: "Witness"     },
                    { id: "jud", label: "Judge"       }
                ]

                Button {
                    text: modelData.label
                    checkable: true
                    checked: root.controller ? root.controller.side === modelData.id : false
                    onClicked: {
                        if (root.controller)
                            root.controller.selectSide(modelData.id)
                    }
                    Layout.fillWidth: true
                }
            }
        }
    }
}
