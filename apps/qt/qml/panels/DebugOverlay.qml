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

    // Palette mirrored from the SDL widget for visual parity across frontends.
    readonly property var sliceColors: [
        "#64c864", "#6496ff", "#ffb450",
        "#c864c8", "#ffff64", "#64dcdc",
        "#ff6464"
    ]

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

            // ── Tick Breakdown (pie + legend) ───────────────────
            SectionHeader { text: "Tick Breakdown"; visible: pie.hasData }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8 * root.sc
                visible: pie.hasData

                // Pie chart
                Canvas {
                    id: pie
                    readonly property var sections: dc ? dc.tickSections : []
                    readonly property bool hasData: sections && sections.length > 0

                    implicitWidth: 64 * root.sc
                    implicitHeight: 64 * root.sc
                    Layout.alignment: Qt.AlignTop

                    Connections {
                        target: dc
                        function onStatsChanged() { pie.requestPaint() }
                    }
                    onSectionsChanged: requestPaint()

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        var w = width, h = height
                        var cx = w / 2, cy = h / 2
                        var r = Math.min(w, h) / 2 - 2

                        var total = 0
                        for (var i = 0; i < sections.length; i++)
                            total += sections[i].avgUs
                        if (total <= 0) return

                        var start = -Math.PI / 2 // 12 o'clock
                        for (var j = 0; j < sections.length; j++) {
                            var frac = sections[j].avgUs / total
                            var end = start + frac * Math.PI * 2

                            ctx.beginPath()
                            ctx.moveTo(cx, cy)
                            ctx.arc(cx, cy, r, start, end, false)
                            ctx.closePath()
                            ctx.fillStyle = root.sliceColors[j % root.sliceColors.length]
                            ctx.fill()

                            start = end
                        }

                        // Outline
                        ctx.beginPath()
                        ctx.arc(cx, cy, r, 0, Math.PI * 2)
                        ctx.strokeStyle = "#505050"
                        ctx.lineWidth = 1
                        ctx.stroke()
                    }
                }

                // Legend
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: 1 * root.sc

                    Repeater {
                        model: pie.sections
                        delegate: RowLayout {
                            required property var modelData
                            required property int index
                            spacing: 4 * root.sc
                            Layout.fillWidth: true

                            Rectangle {
                                width: 10 * root.sc; height: 10 * root.sc
                                color: root.sliceColors[index % root.sliceColors.length]
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Label {
                                text: modelData.name
                                font.pixelSize: root.fs
                                color: root.lblColor
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                            Label {
                                text: modelData.avgUs.toFixed(0) + " us"
                                font.pixelSize: root.fs
                                font.family: "monospace"
                                color: root.valColor
                            }
                        }
                    }
                }
            }

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

            // Search box
            TextField {
                id: cacheSearch
                Layout.fillWidth: true
                implicitHeight: 28 * root.sc
                font.pixelSize: root.fs
                placeholderText: "Search…"
                color: root.valColor
                background: Rectangle { color: "#20ffffff"; radius: 2 * root.sc }
            }

            // List (left) + preview (right)
            RowLayout {
                Layout.fillWidth: true
                spacing: 4 * root.sc

                Rectangle {
                    Layout.preferredWidth: 260 * root.sc
                    Layout.preferredHeight: 200 * root.sc
                    color: "#20ffffff"
                    radius: 2 * root.sc

                    ListView {
                        id: cacheListView
                        anchors.fill: parent
                        anchors.margins: 2 * root.sc
                        clip: true
                        model: dc ? dc.cacheList : []
                        currentIndex: -1
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: ListView.view.width
                            height: rowLabel.implicitHeight + 2 * root.sc
                            color: ListView.isCurrentItem ? "#4080cbc4" : "transparent"
                            visible: cacheSearch.text === "" ||
                                     modelData.path.toLowerCase().indexOf(cacheSearch.text.toLowerCase()) !== -1

                            Label {
                                id: rowLabel
                                anchors.fill: parent
                                anchors.leftMargin: 4 * root.sc
                                anchors.rightMargin: 4 * root.sc
                                verticalAlignment: Text.AlignVCenter
                                text: modelData.path + "  [" + modelData.format + "] " +
                                      modelData.bytesFmt + "  (refs:" + modelData.useCount + ")"
                                elide: Text.ElideRight
                                color: root.valColor
                                font.pixelSize: root.fs * 0.9
                                font.family: "monospace"
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    cacheListView.currentIndex = index
                                    if (dc) dc.selectedCachePath = modelData.path
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 160 * root.sc
                    Layout.preferredHeight: 200 * root.sc
                    color: "#20ffffff"
                    radius: 2 * root.sc

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 4 * root.sc
                        spacing: 2 * root.sc

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 110 * root.sc

                            Image {
                                id: preview
                                anchors.centerIn: parent
                                fillMode: Image.PreserveAspectFit
                                smooth: false
                                asynchronous: true
                                cache: false
                                width: parent.width
                                height: parent.height
                                source: (dc && dc.selectedCachePath)
                                        ? "image://cachepreview/" + dc.selectedCachePath
                                        : ""
                                visible: status === Image.Ready
                            }

                            Label {
                                anchors.centerIn: parent
                                text: "(no preview)"
                                color: root.lblColor
                                font.pixelSize: root.fs
                                visible: preview.status !== Image.Ready
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: dc ? dc.selectedCachePath : ""
                            color: root.valColor
                            font.pixelSize: root.fs * 0.85
                            font.family: "monospace"
                            wrapMode: Text.WrapAnywhere
                            maximumLineCount: 2
                            elide: Text.ElideMiddle
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: dc && dc.selectedCacheWidth > 0
                            text: dc ? dc.selectedCacheWidth + "x" + dc.selectedCacheHeight +
                                       (dc.selectedCacheFrameCount > 1 ? " • " + dc.selectedCacheFrameCount + " frames" : "")
                                     : ""
                            color: root.lblColor
                            font.pixelSize: root.fs * 0.85
                        }
                    }
                }
            }

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
