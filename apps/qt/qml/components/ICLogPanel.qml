import QtQuick
import QtQuick.Controls

/**
 * In-character (IC) message log panel.
 * Populated from ICLogEvent in a future pass; placeholder shows a scrollable
 * text area until the ICLog model is wired up.
 */
Frame {
    id: root
    padding: 4

    ScrollView {
        anchors.fill: parent
        clip: true

        TextArea {
            id: log
            readOnly: true
            wrapMode: Text.Wrap
            font.pixelSize: 11
            text: ""
            background: null
        }
    }
}
