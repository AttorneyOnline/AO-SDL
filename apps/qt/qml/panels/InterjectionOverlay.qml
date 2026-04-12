import QtQuick
import QtQuick.Controls

/**
 * Full-screen interjection overlay (OBJECTION!, HOLD IT!, TAKE THAT!).
 * Shown briefly when the engine emits a corresponding ICLogEvent with an
 * interjection tag; wired up in a future pass.
 */
Item {
    id: root
    width: 400
    height: 120
    visible: false

    property string text: ""

    function show(word) {
        root.text = word;
        root.visible = true;
        hideTimer.restart();
    }

    Timer {
        id: hideTimer
        interval: 2000
        onTriggered: root.visible = false
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    Label {
        anchors.centerIn: parent
        text: root.text
        font.pixelSize: 52
        font.bold: true
        color: "red"
        style: Text.Outline
        styleColor: "white"
    }
}
