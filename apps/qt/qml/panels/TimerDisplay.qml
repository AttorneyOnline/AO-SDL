import QtQuick
import QtQuick.Controls

/**
 * Server-controlled countdown timer display.
 * TimerEvent integration is wired in a future pass.
 */
Item {
    id: root
    width: timerLabel.implicitWidth + 16
    height: timerLabel.implicitHeight + 8

    property int secondsRemaining: 0
    readonly property bool active: secondsRemaining > 0

    visible: active

    Rectangle {
        anchors.fill: parent
        color: "#aa000000"
        radius: 4
    }

    Label {
        id: timerLabel
        anchors.centerIn: parent
        text: {
            var m = Math.floor(root.secondsRemaining / 60);
            var s = root.secondsRemaining % 60;
            return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s;
        }
        color: "white"
        font.pixelSize: 14
        font.bold: true
    }
}
