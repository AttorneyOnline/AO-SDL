import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
/**
 * Main courtroom screen.  The game scene is rendered by the SceneTextureItem
 * in Main.qml behind all screens — this page only hosts the courtroom HUD
 * overlays.
 */
Page {
    id: root
    padding: 0
    background: null

    // HUD overlay
    HealthBars {
        anchors {
            left: parent.left
            top:  parent.top
            margins: 4
        }
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

    // Right-side panel stack
    ColumnLayout {
        anchors {
            right:   parent.right
            top:     parent.top
            bottom:  parent.bottom
            margins: 4
        }
        width: 220
        spacing: 4

        PlayerList   { Layout.fillWidth: true; Layout.preferredHeight: 120 }
        MusicArea    { Layout.fillWidth: true; Layout.fillHeight:      true }
        ChatPanel    { Layout.fillWidth: true; Layout.preferredHeight: 160 }
    }

    // Bottom panel
    ICChatInput {
        anchors {
            left:   parent.left
            right:  parent.right
            bottom: parent.bottom
            margins: 4
        }
        height: 100
    }

    // IC log overlay (toggleable)
    ICLogPanel {
        id: icLog
        visible: false
        anchors {
            left:   parent.left
            top:    parent.top
            bottom: icChatInput.top
            margins: 4
        }
        width: parent.width * 0.45
    }

    // Evidence + emote panels (hidden by default, shown via toolbar)
    EvidencePanel   { id: evidencePanel;  visible: false; anchors.centerIn: parent; width: 320; height: 260 }
    EmoteSelector   { id: emoteSelector; visible: false; anchors.centerIn: parent; width: 400; height: 300 }
    MessageOptions  { id: msgOptions;    visible: false; anchors.centerIn: parent; width: 280; height: 200 }
    SideSelect      { id: sideSelect;    visible: false; anchors.centerIn: parent; width: 240; height: 120 }
    DisconnectModal { id: disconnectModal }
    DebugOverlay    { id: debugOverlay;   visible: false }
}
