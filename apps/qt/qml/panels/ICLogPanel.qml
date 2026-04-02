import QtQuick
import QtQuick.Controls

/**
 * In-character (IC) message log panel.
 * Placeholder scrollable text area until ICLog model is wired to controller.
 */
Frame {
    id: root
    required property var controller
    padding: 4

    ScrollView {
        anchors.fill: parent
        clip: true

        TextArea {
            readOnly: true
            wrapMode: Text.Wrap
            font.pixelSize: 11
            text: ""
            background: null
        }
    }
}
