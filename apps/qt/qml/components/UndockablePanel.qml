import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Wrapper that lets any panel be popped out into a floating tool window.
 *
 * Usage:
 *   UndockablePanel {
 *       title:        "Music & Areas"
 *       undockedSize: Qt.size(280, 400)
 *       panelComponent: Component {
 *           MusicAreaPanel { controller: root.controller }
 *       }
 *   }
 *
 * When docked (default) the content renders inline beneath a slim title bar
 * that carries an undock button (⤢).  When undocked, a floating Qt.Tool
 * window holds the content and the inline slot shows a re-dock placeholder.
 * Closing the floating window re-docks automatically.
 *
 * The floating window respects the parent panel's visibility: hiding the
 * UndockablePanel also hides the detached window.
 */
Item {
    id: root

    property string    title:        ""
    property bool      docked:       true
    property size      undockedSize: Qt.size(300, 240)
    property Component panelComponent: null

    // ── Docked view ────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        visible: root.docked
        spacing: 0

        // Title / toolbar strip
        Rectangle {
            Layout.fillWidth: true
            height: 22
            color: "#2a2a2a"

            RowLayout {
                anchors { fill: parent; leftMargin: 6; rightMargin: 2 }
                spacing: 4

                Label {
                    text: root.title
                    font.bold: true
                    font.pixelSize: 10
                    color: "#cccccc"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                ToolButton {
                    text: "⤢"
                    font.pixelSize: 11
                    implicitWidth:  20
                    implicitHeight: 20
                    ToolTip.text:    "Undock"
                    ToolTip.visible: hovered
                    onClicked: root.docked = false
                }
            }
        }

        // Inline content — active only while docked
        Loader {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            active:            root.docked
            sourceComponent:   root.panelComponent
        }
    }

    // ── Undocked placeholder (shown in the slot while the window is open) ──
    Frame {
        anchors.fill: parent
        visible: !root.docked

        Column {
            anchors.centerIn: parent
            spacing: 6

            Label {
                text: root.title
                font.pixelSize: 10
                opacity: 0.45
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Button {
                text: "↩  Re-dock"
                font.pixelSize: 10
                implicitHeight: 22
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: floatWin.close()
            }
        }
    }

    // ── Floating window ────────────────────────────────────────────────
    Window {
        id: floatWin
        title:   root.title
        // Respects both the docked state and the panel's own visibility so
        // that toggling the parent panel hides the detached window too.
        visible: !root.docked && root.visible
        width:   root.undockedSize.width
        height:  root.undockedSize.height
        flags:   Qt.Tool | Qt.WindowTitleHint | Qt.WindowCloseButtonHint

        onClosing: root.docked = true

        // Active only while undocked — mirrors the lifecycle of the docked Loader
        Loader {
            anchors.fill: parent
            active:          !root.docked
            sourceComponent: root.panelComponent
        }
    }
}
