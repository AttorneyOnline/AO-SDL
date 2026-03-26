import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Side selection panel: Defence, Prosecution, Witness, Judge.
 */
Frame {
    id: root
    padding: 4

    property string selectedSide: "def"

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        Label { text: "Choose your side"; font.bold: true; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter }

        GridLayout {
            columns: 2
            Layout.fillWidth: true
            columnSpacing: 6
            rowSpacing: 6

            Repeater {
                model: [
                    { id: "def", label: "Defence"      },
                    { id: "pro", label: "Prosecution"  },
                    { id: "wit", label: "Witness"      },
                    { id: "jud", label: "Judge"        }
                ]

                Button {
                    text: modelData.label
                    checkable: true
                    checked: root.selectedSide === modelData.id
                    onClicked: root.selectedSide = modelData.id
                    Layout.fillWidth: true
                }
            }
        }
    }
}
