import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import AO.Components
import AO.Panels

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "Attorney Online"
    color: "#000"

    // ── ExpandingBox demo — toggled with F11 ───────────────────────
    Window {
        id: demoWindow
        visible: false
        width: 700
        height: 700
        title: "ExpandingBox Demo"
        color: "#1e1e1e"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 0

            // ── Vertical examples ────────────────────────────────────
            Label {
                text: "Vertical"
                color: "white"
                font.pixelSize: 12
                Layout.bottomMargin: 4
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                ExpandingBox {
                    expandedHeight: 160
                    buttonComponent: Component {
                        Button { text: "Filters" }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        CheckBox { text: "Show taken"; checked: true }
                        CheckBox { text: "Show empty" }
                        CheckBox { text: "Favourites only" }
                    }
                }

                ExpandingBox {
                    expandedHeight: 200
                    buttonComponent: Component {
                        Button { text: "Server Info" }
                    }

                    Label {
                        anchors.fill: parent
                        anchors.margins: 8
                        wrapMode: Text.Wrap
                        color: "white"
                        text: "This area would show server details. "
                            + "It expands and collapses, releasing space "
                            + "to the other items in the layout."
                    }
                }

                ExpandingBox {
                    expandedHeight: 140
                    buttonComponent: Component {
                        Button { text: "Quick Settings" }
                    }

                    GridLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        columns: 2
                        Label { text: "Volume"; color: "white" }
                        Slider { Layout.fillWidth: true }
                        Label { text: "Speed"; color: "white" }
                        Slider { Layout.fillWidth: true }
                    }
                }
            }

            // ── Horizontal examples ──────────────────────────────────
            Label {
                text: "Horizontal"
                color: "white"
                font.pixelSize: 12
                Layout.topMargin: 12
                Layout.bottomMargin: 4
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 160
                spacing: 4

                ExpandingBox {
                    expandedWidth: 220
                    buttonComponent: Component {
                        Button { text: "\u25B6" }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        Label { text: "Sidebar panel"; color: "white"; font.bold: true }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: ["Lobby", "Defence", "Prosecution", "Judge", "Witness"]
                            delegate: ItemDelegate {
                                width: ListView.view.width
                                text: modelData
                            }
                        }
                    }
                }

                ExpandingBox {
                    expandedWidth: 180
                    buttonComponent: Component {
                        Button { text: "\u25B6" }
                    }

                    GridLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        columns: 1
                        Label { text: "Tool palette"; color: "white"; font.bold: true }
                        Button { text: "Objection!"; Layout.fillWidth: true }
                        Button { text: "Hold It!"; Layout.fillWidth: true }
                        Button { text: "Take That!"; Layout.fillWidth: true }
                    }
                }

                // Spacer — takes remaining width
                Item { Layout.fillWidth: true }
            }

            // ── Bi-directional examples ──────────────────────────────
            Label {
                text: "Bi-directional (bottom-right)"
                color: "white"
                font.pixelSize: 12
                Layout.topMargin: 12
                Layout.bottomMargin: 4
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                ExpandingBox {
                    expandedWidth:  280
                    expandedHeight: 200
                    buttonComponent: Component {
                        Button { text: "Evidence" }
                    }

                    GridLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        columns: 3
                        Repeater {
                            model: 6
                            delegate: Rectangle {
                                width: 72; height: 72
                                color: "#444"
                                radius: 4
                                Label {
                                    anchors.centerIn: parent
                                    text: "Img " + (index + 1)
                                    color: "white"
                                }
                            }
                        }
                    }
                }

                ExpandingBox {
                    expandedWidth:  200
                    expandedHeight: 180
                    buttonComponent: Component {
                        Button { text: "Notes" }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        Label { text: "Case notes"; color: "white"; font.bold: true }
                        TextArea {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            placeholderText: "Type notes here..."
                            wrapMode: TextEdit.Wrap
                            color: "white"
                        }
                    }
                }

                // Spacer
                Item { Layout.fillWidth: true; Layout.fillHeight: true }
            }

            // ── Inverted examples ────────────────────────────────────
            Label {
                text: "Inverted (expand leftward / upward)"
                color: "white"
                font.pixelSize: 12
                Layout.topMargin: 12
                Layout.bottomMargin: 4
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 140
                spacing: 4

                // Spacer — pushes boxes to the right
                Item { Layout.fillWidth: true }

                ExpandingBox {
                    expandedWidth: 200
                    invertHorizontal: true
                    buttonComponent: Component {
                        Button { text: "\u25C0" }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        Label { text: "Right-to-left panel"; color: "white"; font.bold: true }
                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            color: "white"
                            text: "This expanded from a button on the right edge."
                        }
                    }
                }

                ExpandingBox {
                    expandedWidth:  240
                    expandedHeight: 180
                    invertHorizontal: true
                    invertVertical: true
                    buttonComponent: Component {
                        Button { text: "\u25C4\u25B2" }
                    }

                    GridLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        columns: 2
                        Label { text: "Top-left expand"; color: "white"; font.bold: true; Layout.columnSpan: 2 }
                        Button { text: "Action A"; Layout.fillWidth: true }
                        Button { text: "Action B"; Layout.fillWidth: true }
                        Button { text: "Action C"; Layout.fillWidth: true }
                        Button { text: "Action D"; Layout.fillWidth: true }
                    }
                }
            }
        }
    }

    // ── AccordionDrawer demo — toggled with F9 ─────────────────────
    Window {
        id: accordionDemoWindow
        visible: false
        width: 560
        height: 600
        title: "AccordionDrawer Demo"
        color: "#1e1e1e"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            // ── Title + direction selector ───────────────────────────
            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: "AccordionDrawer"
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: "Direction:"
                    color: "#999999"
                    font.pixelSize: 11
                }

                ComboBox {
                    id: directionPicker
                    // Display names match enum order: 0=TopToBottom … 3=RightToLeft
                    model: ["Top to Bottom", "Bottom to Top", "Left to Right", "Right to Left"]
                    currentIndex: 0
                    implicitWidth: 140
                }
            }

            Label {
                text: "Only one section can be open at a time.  Change the direction above to see all four orientations."
                color: "#999999"
                font.pixelSize: 11
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            // ── Live preview — recreated whenever direction changes ───
            // A Loader lets us swap the whole AccordionDrawer (and its
            // children) so the new direction takes effect cleanly.
            Loader {
                id: accordionLoader
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Rebuild when direction changes so sections re-parent
                // into the correct Column / Row from scratch.
                // currentIndex maps directly to the AccordionDrawer.Direction enum values.
                property int chosenDirection: directionPicker.currentIndex
                onChosenDirectionChanged: { sourceComponent = null; sourceComponent = accordionComponent }
                Component.onCompleted:    sourceComponent = accordionComponent

                Component {
                    id: accordionComponent

                    AccordionDrawer {
                        direction: accordionLoader.chosenDirection
                        expandedIndex: 0

                        AccordionSection {
                            title: "General Settings"

                            ColumnLayout {
                                anchors { left: parent.left; right: parent.right }
                                anchors.margins: 8
                                spacing: 4

                                CheckBox { text: "Show timestamps"; checked: true }
                                CheckBox { text: "Enable notifications" }
                                CheckBox { text: "Auto-reconnect"; checked: true }
                            }
                        }

                        AccordionSection {
                            title: "Audio"

                            GridLayout {
                                anchors { left: parent.left; right: parent.right }
                                anchors.margins: 8
                                columns: 2
                                columnSpacing: 8
                                rowSpacing: 4

                                Label { text: "Music"; color: "white" }
                                Slider { Layout.fillWidth: true; value: 0.7 }
                                Label { text: "SFX"; color: "white" }
                                Slider { Layout.fillWidth: true; value: 0.9 }
                                Label { text: "Blips"; color: "white" }
                                Slider { Layout.fillWidth: true; value: 0.5 }
                            }
                        }

                        AccordionSection {
                            title: "Appearance"

                            ColumnLayout {
                                anchors { left: parent.left; right: parent.right }
                                anchors.margins: 8
                                spacing: 4

                                Label { text: "Theme"; color: "white"; font.bold: true }
                                ComboBox {
                                    Layout.fillWidth: true
                                    model: ["Dark", "Light", "Classic"]
                                }

                                Label { text: "Font size"; color: "white"; font.bold: true }
                                SpinBox { from: 8; to: 24; value: 12 }
                            }
                        }

                        AccordionSection {
                            title: "About"

                            Label {
                                anchors { left: parent.left; right: parent.right; margins: 8 }
                                wrapMode: Text.Wrap
                                color: "#cccccc"
                                text: "Attorney Online — open-source courtroom simulator.\n\n"
                                    + "Select a direction above to see the accordion open "
                                    + "in all four orientations."
                            }
                        }
                    }
                }
            }
        }
    }

    // ── QML Playground — toggled with F8 ──────────────────────────
    Window {
        id: playgroundWindow
        visible: false
        width: 900
        height: 650
        title: "QML Playground"
        color: "#1e1e1e"

        // ── State ────────────────────────────────────────────────────
        property url   loadedUrl:   ""
        property string statusText: "No file loaded.  Click Browse or paste a path and press Load."
        property bool  hasError:    false

        function loadUrl(url) {
            if (!url || url.toString() === "") return
            loadedUrl   = url
            hasError    = false
            statusText  = "Loading…"
            previewLoader.source = ""        // force reload even if same url
            previewLoader.source = url
        }

        // Convert a plain filesystem path typed by the user into a file:// URL.
        function pathToUrl(path) {
            let s = path.trim()
            if (s === "") return ""
            if (s.startsWith("file://")) return s
            // Windows absolute path  C:\… or C:/…
            if (/^[A-Za-z]:/.test(s)) return "file:///" + s.replace(/\\/g, "/")
            return "file://" + s
        }

        // ── File picker dialog ───────────────────────────────────────
        FileDialog {
            id: filePicker
            title: "Open QML File"
            nameFilters: ["QML files (*.qml)", "All files (*)"]
            onAccepted: {
                pathField.text = selectedFile.toString()
                playgroundWindow.loadUrl(selectedFile)
            }
        }

        // ── Layout ───────────────────────────────────────────────────
        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // ── Toolbar ──────────────────────────────────────────────
            Rectangle {
                color: "#252525"
                Layout.fillWidth: true
                implicitHeight: toolRow.implicitHeight + 12

                RowLayout {
                    id: toolRow
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    Button {
                        text: "Browse…"
                        onClicked: filePicker.open()
                    }

                    TextField {
                        id: pathField
                        placeholderText: "Paste a .qml path here, then press Load"
                        color: "#cccccc"
                        Layout.fillWidth: true
                        palette.base: "#1a1a1a"
                        onAccepted: playgroundWindow.loadUrl(playgroundWindow.pathToUrl(text))
                    }

                    Button {
                        text: "Load"
                        onClicked: playgroundWindow.loadUrl(playgroundWindow.pathToUrl(pathField.text))
                    }

                    Button {
                        text: "Reload"
                        enabled: playgroundWindow.loadedUrl.toString() !== ""
                        onClicked: playgroundWindow.loadUrl(playgroundWindow.loadedUrl)
                    }
                }
            }

            // ── Preview area ─────────────────────────────────────────
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                // Checker-board background so transparent items are visible.
                Rectangle {
                    width:  Math.max(previewLoader.implicitWidth  + 40, playgroundWindow.width)
                    height: Math.max(previewLoader.implicitHeight + 40, playgroundWindow.height - toolRow.height - statusBar.height - 40)
                    color: "transparent"

                    // Checker pattern via canvas (avoids Grid item-count warnings on resize).
                    Canvas {
                        anchors.fill: parent
                        property int sz: 16
                        onPaint: {
                            var ctx = getContext("2d");
                            var cols = Math.ceil(width / sz);
                            var rows = Math.ceil(height / sz);
                            for (var r = 0; r < rows; r++) {
                                for (var c = 0; c < cols; c++) {
                                    ctx.fillStyle = ((r + c) % 2) ? "#2a2a2a" : "#222222";
                                    ctx.fillRect(c * sz, r * sz, sz, sz);
                                }
                            }
                        }
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                    }

                    // The user's loaded component, centered.
                    Loader {
                        id: previewLoader
                        anchors.centerIn: parent

                        onStatusChanged: {
                            switch (status) {
                            case Loader.Ready:
                                playgroundWindow.hasError   = false
                                playgroundWindow.statusText = "Loaded: " + playgroundWindow.loadedUrl.toString().replace(/.*\//, "")
                                break
                            case Loader.Error:
                                playgroundWindow.hasError   = true
                                playgroundWindow.statusText = errorString
                                break
                            case Loader.Loading:
                                playgroundWindow.statusText = "Loading…"
                                break
                            default:
                                break
                            }
                        }
                    }

                    // Placeholder shown before any file is loaded.
                    Label {
                        visible: previewLoader.status === Loader.Null
                        anchors.centerIn: parent
                        text: "Open a .qml file to preview it here.\n\nAll AO.Components and AO.Panels imports are available."
                        horizontalAlignment: Text.AlignHCenter
                        color: "#555555"
                        font.pixelSize: 13
                    }
                }
            }

            // ── Status bar ───────────────────────────────────────────
            Rectangle {
                id: statusBar
                color: "#1a1a1a"
                Layout.fillWidth: true
                implicitHeight: statusLabel.implicitHeight + 8

                Label {
                    id: statusLabel
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    text: playgroundWindow.statusText
                    color: playgroundWindow.hasError ? "#e06c75" : "#888888"
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                }
            }
        }
    }

    // ── Normal app below ─────────────────────────────────────────────

    // Navigation: swap screens by mapping UIManager's active screen id.
    Loader {
        anchors.fill: parent
        source: {
            switch (app.currentScreenId) {
            case "server_list":
                return "screens/ServerListScreen.qml";
            case "char_select":
                return "screens/CharSelectScreen.qml";
            case "courtroom":
                return "screens/CourtroomScreen.qml";
            default:
                return "";
            }
        }
    }

    // Debug overlay — toggled with F12.
    DebugOverlay {
        id: debugOverlay
        visible: false
    }

    Shortcut {
        sequence: "F8"
        onActivated: playgroundWindow.visible = !playgroundWindow.visible
    }

    Shortcut {
        sequence: "F9"
        onActivated: accordionDemoWindow.visible = !accordionDemoWindow.visible
    }

    Shortcut {
        sequence: "F11"
        onActivated: demoWindow.visible = !demoWindow.visible
    }

    Shortcut {
        sequence: "F12"
        onActivated: debugOverlay.visible = !debugOverlay.visible
    }
}
