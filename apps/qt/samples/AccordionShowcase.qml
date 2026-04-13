import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AO.Components

/**
 * All four AccordionDrawer directions rendered side by side.
 *
 * Demonstrates how the same AccordionDrawer + AccordionSection API
 * adapts to vertical and horizontal orientations.  Each quadrant
 * is a self-contained drawer with its own content.
 */
Rectangle {
    id: root
    width: 640; height: 480
    color: "#141414"
    radius: 8

    // ── Helper for section content ─────────────────────────────────
    component InfoBlock: ColumnLayout {
        required property string heading
        required property var    items

        spacing: 4

        Label {
            text: heading
            color: "#e0e0e0"
            font { pixelSize: 12; bold: true }
            Layout.fillWidth: true
        }

        Repeater {
            model: items
            Label {
                required property string modelData
                text: "\u2022  " + modelData
                color: "#999"
                font.pixelSize: 11
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 12
        columns: 2
        rowSpacing: 10
        columnSpacing: 10

        // ── Top-left: TopToBottom ───────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Label { text: "TopToBottom"; color: "#4fc3f7"; font { pixelSize: 11; bold: true } }

            AccordionDrawer {
                direction: AccordionDrawer.TopToBottom
                expandedIndex: 0

                AccordionSection {
                    title: "Prosecution"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Witnesses"
                        items: ["Larry Butz", "Angel Starr", "Lotta Hart"]
                    }
                }
                AccordionSection {
                    title: "Defense"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Evidence"
                        items: ["Attorney Badge", "Autopsy Report", "Glass Shards"]
                    }
                }
                AccordionSection {
                    title: "Judge"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Rulings"
                        items: ["Sustained", "Overruled"]
                    }
                }
            }
        }

        // ── Top-right: BottomToTop ─────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Label { text: "BottomToTop"; color: "#81c784"; font { pixelSize: 11; bold: true } }

            AccordionDrawer {
                direction: AccordionDrawer.BottomToTop
                expandedIndex: 0

                AccordionSection {
                    title: "Music"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Now Playing"
                        items: ["Trial - Pursuit", "Objection! 2001"]
                    }
                }
                AccordionSection {
                    title: "SFX"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Recent"
                        items: ["Gavel", "Realization", "Damage"]
                    }
                }
                AccordionSection {
                    title: "Ambience"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Loops"
                        items: ["Courtroom", "Lobby", "Detention"]
                    }
                }
            }
        }

        // ── Bottom-left: LeftToRight ───────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Label { text: "LeftToRight"; color: "#ffb74d"; font { pixelSize: 11; bold: true } }

            AccordionDrawer {
                direction: AccordionDrawer.LeftToRight
                expandedIndex: 0

                AccordionSection {
                    title: "Chat"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Channels"
                        items: ["IC", "OOC", "Music"]
                    }
                }
                AccordionSection {
                    title: "Players"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Online"
                        items: ["Phoenix", "Edgeworth", "Godot"]
                    }
                }
            }
        }

        // ── Bottom-right: RightToLeft ──────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Label { text: "RightToLeft"; color: "#ce93d8"; font { pixelSize: 11; bold: true } }

            AccordionDrawer {
                direction: AccordionDrawer.RightToLeft
                expandedIndex: 0

                AccordionSection {
                    title: "Log"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Events"
                        items: ["Case opened", "Witness called", "Cross-exam started"]
                    }
                }
                AccordionSection {
                    title: "Notes"
                    InfoBlock {
                        anchors { left: parent.left; right: parent.right; margins: 8 }
                        heading: "Saved"
                        items: ["Contradiction in testimony", "Check photo timestamp"]
                    }
                }
            }
        }
    }
}
