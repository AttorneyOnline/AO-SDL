import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Developer debug overlay — toggled with F12.
 * Reads from app.debugController Q_PROPERTYs.
 * Scales proportionally to the window height so it stays readable at any resolution.
 */
Rectangle {
    id: root
    color: "#DD101020"
    radius: 4 * sc
    width: col.implicitWidth + 16 * sc
    anchors.top: parent.top
    anchors.right: parent.right
    anchors.margins: 8 * sc
    height: Math.min(col.implicitHeight + 16 * sc, parent.height - 16 * sc)

    // Scale factor: 1.0 at 720p, ~1.5 at 1080p, ~2.0 at 1440p
    readonly property real sc: parent.height / 720

    readonly property var dc: app.debugController
    readonly property color hdrColor: "#80cbc4"
    readonly property color lblColor: "#b0b0b0"
    readonly property color valColor: "#e0e0e0"
    readonly property real fs: 26 * sc

    Flickable {
        anchors.fill: parent
        anchors.margins: 6 * root.sc
        contentHeight: col.implicitHeight
        clip: true
        flickableDirection: Flickable.VerticalFlick

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 1 * root.sc

            // ── Performance ─────────────────────────────────────
            SectionHeader { text: "Performance" }
            DebugRow { label: "FPS"; value: dc ? dc.fps.toFixed(1) : "?" }
            DebugRow { label: "Game tick"; value: dc ? dc.gameTickMs.toFixed(2) + " ms" : "?" }
            DebugRow { label: "Tick rate"; value: dc ? dc.tickRateHz.toFixed(1) + " Hz" : "?" }

            DebugSep {}

            // ── Renderer ────────────────────────────────────────
            SectionHeader { text: "Renderer" }
            DebugRow { label: "Backend"; value: dc ? dc.backendName : "?" }
            RowLayout {
                spacing: 4 * root.sc
                Layout.fillWidth: true
                Label { text: "Scale:"; font.pixelSize: root.fs; color: root.lblColor }
                SpinBox {
                    from: 1; to: 8
                    value: dc ? dc.internalScale : 4
                    font.pixelSize: root.fs
                    implicitWidth: 80 * root.sc; implicitHeight: 26 * root.sc
                    onValueModified: { if (dc) dc.internalScale = value }
                }
                Item { Layout.fillWidth: true }
                Label { text: "Wire:"; font.pixelSize: root.fs; color: root.lblColor }
                CheckBox {
                    checked: dc ? dc.wireframe : false
                    onToggled: { if (dc) dc.wireframe = checked }
                    implicitHeight: 22 * root.sc; implicitWidth: 22 * root.sc
                }
            }

            DebugSep {}

            // ── Connection ──────────────────────────────────────
            SectionHeader { text: "Connection" }
            DebugRow { label: "State"; value: dc ? dc.connState : "?" }
            DebugRow { label: "Server"; value: dc ? (dc.serverInfo || "\u2014") : "?" }
            DebugRow { label: "Players"; value: dc ? dc.currentPlayers + "/" + dc.maxPlayers : "?" }

            DebugSep {}

            // ── HTTP ────────────────────────────────────────────
            SectionHeader { text: "HTTP" }
            RowLayout {
                spacing: 4 * root.sc; Layout.fillWidth: true
                Label { text: "P:"; font.pixelSize: root.fs; color: root.lblColor }
                Label { text: dc ? "" + dc.httpPending : "?"; font.pixelSize: root.fs; font.family: "monospace"; color: root.valColor }
                Label { text: "C:"; font.pixelSize: root.fs; color: root.lblColor }
                Label { text: dc ? "" + dc.httpCached : "?"; font.pixelSize: root.fs; font.family: "monospace"; color: root.valColor }
                Label { text: "F:"; font.pixelSize: root.fs; color: root.lblColor }
                Label { text: dc ? "" + dc.httpFailed : "?"; font.pixelSize: root.fs; font.family: "monospace"; color: root.valColor }
                Item { Layout.fillWidth: true }
                Label { text: dc ? dc.httpCachedSize : "?"; font.pixelSize: root.fs; font.family: "monospace"; color: root.valColor }
            }
            DebugRow { label: "Pool queue"; value: dc ? dc.httpPoolPending : "?" }

            DebugSep {}

            // ── Asset Cache ─────────────────────────────────────
            SectionHeader { text: "Cache" }
            DebugRow { label: "Entries"; value: dc ? dc.cacheEntries : "?" }
            DebugRow { label: "Used / Max"; value: dc ? dc.cacheUsed + " / " + dc.cacheMax : "?" }

            DebugSep {}

            // ── Event Stats ─────────────────────────────────────
            SectionHeader { text: "Events" }
            Repeater {
                model: dc ? dc.eventStats : []
                delegate: RowLayout {
                    required property var modelData
                    spacing: 4 * root.sc
                    Layout.fillWidth: true
                    Label {
                        text: modelData.name
                        font.pixelSize: root.fs
                        color: root.lblColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Label {
                        text: "" + modelData.count
                        font.pixelSize: root.fs
                        font.family: "monospace"
                        color: root.valColor
                    }
                }
            }
        }
    }

    // ── Inline helper components ────────────────────────────────

    component SectionHeader: Label {
        font.bold: true; font.pixelSize: root.fs * 1.05
        color: root.hdrColor
        Layout.topMargin: 2 * root.sc; Layout.bottomMargin: 1 * root.sc
    }

    component DebugRow: RowLayout {
        required property string label
        required property var value
        spacing: 4 * root.sc
        Layout.fillWidth: true
        Label {
            text: parent.label + ":"
            font.pixelSize: root.fs
            color: root.lblColor
        }
        Label {
            text: "" + parent.value
            font.pixelSize: root.fs
            font.family: "monospace"
            color: root.valColor
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
        }
    }

    component DebugSep: Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 3 * root.sc; Layout.bottomMargin: 3 * root.sc
        height: 1
        color: "#30ffffff"
    }
}
