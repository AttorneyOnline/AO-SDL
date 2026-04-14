import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import AO.Components
import AO.Panels

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
 *
 * Each panel receives its specific controller from app.* rather than a
 * single monolithic courtroomController.
 */
Page {
    id: root
    padding: 0
    background: null

    // ── Bottom: IC composition bar ─────────────────────────────────────────
    ICInputPanel {
        id: icInput
        controller: app.icController
        anchors {
            left:    parent.left
            right:   parent.right
            bottom:  parent.bottom
            margins: 4
        }
        height: 100

        onEmoteRequested:    emoteOverlay.visible   = !emoteOverlay.visible
        onEvidenceRequested: evidenceOverlay.visible = !evidenceOverlay.visible
        onSideRequested:     sideOverlay.visible     = !sideOverlay.visible
        onOptionsRequested:  msgOverlay.visible      = !msgOverlay.visible
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
        SplitView {
            id: rightSidebar
            orientation: Qt.Vertical
            anchors {
                right:  parent.right
                top:    parent.top
                bottom: parent.bottom
            }
            width: 220

            PlayerListPanel {
                controller: app.playerController
                SplitView.preferredHeight: 120
                SplitView.minimumHeight:   60
            }

            DockPanel {
                title:             "Music & Areas"
                dockedHeight:      200
                undockedSize:      Qt.size(280, 400)
                panelComponent: Component {
                    MusicAreaPanel {
                        controller:      app.musicAreaController
                        audioController: app.audioController
                    }
                }
            }

            OOCChatPanel {
                controller: app.chatController
                oocName:    app.icController.charName
                SplitView.fillHeight: true
                SplitView.minimumHeight: 60
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

            // IC log toggle — floats at the top-left corner of the viewport zone.
            RoundButton {
                id: icLogToggle
                text: "≡"
                font.pixelSize: 14
                width: 28; height: 28
                opacity: 0.7
                anchors { left: parent.left; top: parent.top; margins: 4 }
                onClicked: icLog.visible = !icLog.visible
                ToolTip.visible: hovered
                ToolTip.text: "Toggle IC log"
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
                    controller: app.hudController
                    anchors { left: parent.left; top: parent.top; margins: 4 }
                }

                TimerDisplay {
                    secondsRemaining: app.hudController.timerSeconds
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top:              parent.top
                        topMargin:        4
                    }
                }

                InterjectionOverlay {
                    id: interjectionOverlay
                    anchors.centerIn: parent

                    Connections {
                        target: app.hudController
                        function onInterjectionTriggered(word) {
                            interjectionOverlay.show(word)
                        }
                    }
                }

                // IC log strip — left side, toggled externally (hidden by default)
                ICLogPanel {
                    id: icLog
                    controller: app.icController
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

        DockPanel {
            id: emoteOverlay
            title:        "Emotes"
            overlay:      true
            visible:      false
            undockedSize: Qt.size(400, 300)
            anchors.centerIn: parent
            width: 400; height: 300
            panelComponent: Component {
                EmoteSelectorPanel { controller: app.icController }
            }
        }

        EvidencePanel {
            id: evidenceOverlay
            controller: app.evidenceController
            visible: false
            anchors.centerIn: parent
            width: 320; height: 260
        }

        MessageOptionsPanel {
            id: msgOverlay
            controller: app.icController
            visible: false
            anchors.centerIn: parent
            width: 280; height: 200
        }

        SideSelectPanel {
            id: sideOverlay
            controller: app.icController
            visible: false
            anchors.centerIn: parent
            width: 240; height: 120
        }
    }

    // ── Application-modal dialogs and global overlays ─────────────────────
    DisconnectModal { id: disconnectModal }
}
