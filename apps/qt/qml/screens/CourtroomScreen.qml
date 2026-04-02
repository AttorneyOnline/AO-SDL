import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Courtroom screen.
 *
 * Zone layout (no free-floating overlaps):
 *
 *   ┌──────────────────────────────────────┬─────────────┐
 *   │  viewportArea                         │ rightSidebar│
 *   │  ┌────────────────────────────────┐  │ PlayerList  │
 *   │  │  GameViewport (aspect-scaled)  │  │─────────────│
 *   │  │  HealthPanel    (HUD overlay)  │  │ Music/Areas │
 *   │  │  TimerDisplay   (HUD overlay)  │  │ (undockable)│
 *   │  │  InterjectionOverlay           │  │─────────────│
 *   │  │  ICLogPanel (toggleable strip) │  │ OOC Chat    │
 *   │  └────────────────────────────────┘  │             │
 *   ├──────────────────────────────────────┴─────────────┤
 *   │  ICInputPanel                                       │
 *   └─────────────────────────────────────────────────────┘
 *
 * Overlay panels (EmoteSelector, Evidence, Options, Side) float above
 * mainArea when shown and can be undocked to persistent tool windows.
 */
Page {
    id: root
    padding: 0
    background: null

    property var controller: app.courtroomController

    // ── Bottom: IC composition bar ─────────────────────────────────────────
    ICInputPanel {
        id: icInput
        controller: root.controller
        anchors {
            left:    parent.left
            right:   parent.right
            bottom:  parent.bottom
            margins: 4
        }
        height: 100

        onEmoteRequested:   emoteOverlay.visible = !emoteOverlay.visible
        onSideRequested:    sideOverlay.visible  = !sideOverlay.visible
        onOptionsRequested: msgOverlay.visible   = !msgOverlay.visible
    }

    // ── Main area (fills everything above the IC input bar) ───────────────
    Item {
        id: mainArea
        anchors {
            left:         parent.left
            right:        parent.right
            top:          parent.top
            bottom:       icInput.top
            margins:      4
            bottomMargin: 4
        }

        // ── Right sidebar: player list, music, OOC chat ──────────────────
        ColumnLayout {
            id: rightSidebar
            anchors {
                right:  parent.right
                top:    parent.top
                bottom: parent.bottom
            }
            width: 220
            spacing: 4

            PlayerListPanel {
                controller: root.controller
                Layout.fillWidth:       true
                Layout.preferredHeight: 120
            }

            UndockablePanel {
                title:        "Music & Areas"
                undockedSize: Qt.size(280, 400)
                Layout.fillWidth:  true
                Layout.fillHeight: true
                panelComponent: Component {
                    MusicAreaPanel { controller: root.controller }
                }
            }

            OOCChatPanel {
                controller: root.controller
                Layout.fillWidth:       true
                Layout.preferredHeight: 160
            }
        }

        // ── Viewport area: game scene + HUD ─────────────────────────────
        Item {
            id: viewportArea
            anchors {
                left:        parent.left
                top:         parent.top
                bottom:      parent.bottom
                right:       rightSidebar.left
                rightMargin: 4
            }

            // Game scene — fills the zone; SceneTextureItem scales internally
            // to keep the 256:192 ratio, centring itself within any leftover space.
            GameViewport {
                id: gameViewport
                anchors.fill: parent
            }

            // HUD overlay — positioned exactly over the rendered scene rect,
            // not the surrounding black space.
            Item {
                id: sceneHud
                x:      gameViewport.sceneX
                y:      gameViewport.sceneY
                width:  gameViewport.sceneWidth
                height: gameViewport.sceneHeight

                HealthPanel {
                    controller: root.controller
                    anchors { left: parent.left; top: parent.top; margins: 4 }
                }

                TimerDisplay {
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top:              parent.top
                        topMargin:        4
                    }
                }

                InterjectionOverlay {
                    anchors.centerIn: parent
                }

                // IC log strip — left side, toggled externally (hidden by default)
                ICLogPanel {
                    id: icLog
                    controller: root.controller
                    visible: false
                    anchors {
                        left:    parent.left
                        top:     parent.top
                        bottom:  parent.bottom
                        margins: 4
                    }
                    width: parent.width * 0.45
                }
            }
        }

        // ── Floating overlay panels, centred over mainArea ───────────────
        // Shown via ICInputPanel signals; can be undocked to tool windows.

        UndockablePanel {
            id: emoteOverlay
            title:        "Emotes"
            visible:      false
            undockedSize: Qt.size(400, 300)
            anchors.centerIn: parent
            width: 400; height: 300
            panelComponent: Component {
                EmoteSelectorPanel { controller: root.controller }
            }
        }

        EvidencePanel {
            id: evidenceOverlay
            controller: root.controller
            visible: false
            anchors.centerIn: parent
            width: 320; height: 260
        }

        MessageOptionsPanel {
            id: msgOverlay
            visible: false
            anchors.centerIn: parent
            width: 280; height: 200
        }

        SideSelectPanel {
            id: sideOverlay
            visible: false
            anchors.centerIn: parent
            width: 240; height: 120
        }
    }

    // ── Application-modal dialogs and global overlays ─────────────────────
    DisconnectModal { controller: root.controller }
    DebugOverlay    { visible: false }
}
