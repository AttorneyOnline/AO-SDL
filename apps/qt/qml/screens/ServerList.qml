import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Direct connect bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            TextField {
                id: directConnectInput
                placeholderText: "host:port"
                Layout.fillWidth: true
                onAccepted: connectBtn.clicked()
            }

            Button {
                id: connectBtn
                text: "Connect"
                implicitWidth: 80
                onClicked: {
                    if (!uiBridge || directConnectInput.text.length === 0)
                        return
                    var addr = directConnectInput.text
                    var port = 27016
                    var colon = addr.lastIndexOf(':')
                    if (colon !== -1) {
                        port = parseInt(addr.substring(colon + 1))
                        addr = addr.substring(0, colon)
                    }
                    uiBridge.direct_connect(addr, port)
                }
            }
        }

        // Server table header
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: headerRow.implicitHeight + 8
            color: "#2a2a3e"

            RowLayout {
                id: headerRow
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 0

                Label {
                    text: "Name"
                    font.bold: true
                    color: "#ccc"
                    Layout.preferredWidth: 200
                    Layout.fillWidth: true
                }

                Label {
                    text: "Players"
                    font.bold: true
                    color: "#ccc"
                    Layout.preferredWidth: 60
                    horizontalAlignment: Text.AlignRight
                }

                Label {
                    text: "Description"
                    font.bold: true
                    color: "#ccc"
                    Layout.preferredWidth: 300
                    Layout.fillWidth: true
                    leftPadding: 12
                }
            }
        }

        // Server list
        ListView {
            id: serverList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: uiBridge ? uiBridge.serverListModel : null
            currentIndex: uiBridge ? uiBridge.selectedServer : -1

            // Loading state
            Label {
                anchors.centerIn: parent
                text: "Fetching server list..."
                color: "#888"
                font.pixelSize: 16
                visible: !serverList.model || serverList.count === 0
            }

            delegate: Rectangle {
                id: delegateRoot
                required property int index
                required property string name
                required property int players
                required property string description
                required property bool hasWs

                width: serverList.width
                height: delegateContent.implicitHeight + 8
                color: {
                    if (serverList.currentIndex === index)
                        return "#3a3a5e"
                    return index % 2 === 0 ? "#1e1e2e" : "#24243a"
                }
                opacity: hasWs ? 1.0 : 0.5

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (delegateRoot.hasWs && uiBridge)
                            uiBridge.select_server(delegateRoot.index)
                    }

                    hoverEnabled: true
                    ToolTip.visible: !delegateRoot.hasWs && containsMouse
                    ToolTip.text: "TCP-only server (not supported)"
                }

                RowLayout {
                    id: delegateContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 0

                    Label {
                        text: delegateRoot.name
                        color: "#eee"
                        elide: Text.ElideRight
                        Layout.preferredWidth: 200
                        Layout.fillWidth: true
                    }

                    Label {
                        text: delegateRoot.players
                        color: "#aaa"
                        Layout.preferredWidth: 60
                        horizontalAlignment: Text.AlignRight
                    }

                    Label {
                        text: delegateRoot.description
                        color: "#999"
                        elide: Text.ElideRight
                        Layout.preferredWidth: 300
                        Layout.fillWidth: true
                        leftPadding: 12
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                active: true
            }
        }
    }
}
