import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
        sequence: "F11"
        onActivated: demoWindow.visible = !demoWindow.visible
    }

    Shortcut {
        sequence: "F12"
        onActivated: debugOverlay.visible = !debugOverlay.visible
    }
}
