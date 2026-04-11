import QtQuick
import QtQuick.Controls

/**
 * IC message log panel — scrolling history of in-character messages.
 * controller: ICController — provides icLogModel (showname, message, colorIdx).
 *
 * colorIdx → text colour (AO2 convention):
 *   0 white  1 green  2 red  3 orange  4 blue  5 yellow  6 magenta  7 pink
 */
Frame {
    id: root
    required property var controller
    padding: 4

    // Map AO2 color index to a CSS-compatible colour string.
    readonly property var colorTable: [
        "#ffffff",  // 0 white
        "#00c800",  // 1 green
        "#c80000",  // 2 red
        "#ff8700",  // 3 orange
        "#0087c8",  // 4 blue
        "#c8c800",  // 5 yellow
        "#c800c8",  // 6 magenta
        "#ff87c8",  // 7 pink
    ]

    function colorFor(idx) {
        return (idx >= 0 && idx < colorTable.length) ? colorTable[idx] : colorTable[0]
    }

    ListView {
        id: log
        anchors.fill: parent
        clip: true
        model: root.controller ? root.controller.icLogModel : null
        verticalLayoutDirection: ListView.BottomToTop
        spacing: 2

        delegate: Label {
            width: ListView.view.width
            textFormat: Text.RichText
            wrapMode: Text.Wrap
            font.pixelSize: 11
            text: "<span style='color:%1'><b>%2</b>: %3</span>"
                      .arg(root.colorFor(model.colorIdx))
                      .arg(model.showname.replace(/&/g,"&amp;").replace(/</g,"&lt;"))
                      .arg(model.message .replace(/&/g,"&amp;").replace(/</g,"&lt;"))
        }

        onCountChanged: positionViewAtEnd()
    }
}
