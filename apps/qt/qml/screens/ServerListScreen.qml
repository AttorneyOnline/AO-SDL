import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Server browser screen.
 * Displays the master-server list and a direct-connect bar.
 * Delegates to controller (ServerListController*) for all data and actions.
 */
Page {
    id: root

    property var controller: app.serverListController

    header: Label {
        text: "Attorney Online — Server List"
        font.bold: true
        padding: 8
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Direct connect row
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            TextField {
                id: directField
                Layout.fillWidth: true
                placeholderText: "host:port"
                onAccepted: doDirectConnect()
            }

            Button {
                text: "Connect"
                onClicked: doDirectConnect()
            }
        }

        // Search bar
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "Search servers…"
            onTextChanged: if (root.controller)
                root.controller.setFilter(text)
        }

        // Server list
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.controller ? root.controller.filteredModel : null

            delegate: ItemDelegate {
                width: ListView.view.width
                contentItem: ColumnLayout {
                    spacing: 2
                    Label {
                        text: model.name
                        font.bold: true
                    }
                    Label {
                        text: model.description
                        wrapMode: Text.Wrap
                        opacity: 0.7
                    }
                    Label {
                        text: model.players + " players"
                        font.pixelSize: 11
                    }
                }
                onClicked: root.controller.connectToServer(index)
            }

            Label {
                anchors.centerIn: parent
                visible: listView.count === 0
                text: searchField.text.length ? "No servers match " + searchField.text + "" : "Fetching server list…"
            }
        }
    }

    function doDirectConnect() {
        var addr = directField.text.trim();
        if (!addr.length)
            return;
        var port = 27016;
        var colon = addr.lastIndexOf(":");
        if (colon >= 0) {
            port = parseInt(addr.substring(colon + 1), 10) || port;
            addr = addr.substring(0, colon);
        }
        root.controller.directConnect(addr, port);
        directField.clear();
    }
}
