import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Courtroom overlay — panels sit on top of the GameViewport.
// The uncovered top-left area is where the game scene shows through.
Item {
    id: root

    // Loading overlay
    Rectangle {
        anchors.fill: parent
        color: "#cc000000"
        visible: uiBridge ? uiBridge.courtroomLoading : true
        z: 100

        Label {
            anchors.centerIn: parent
            text: "Loading character data..."
            color: "#ccc"
            font.pixelSize: 18
        }
    }

    // ---- Right panel (Emotes / Music / Evidence / Players) ----
    Rectangle {
        id: rightPanel
        anchors.top: parent.top
        anchors.bottom: bottomPanel.top
        anchors.right: parent.right
        width: 280
        color: "#1a1a2e"
        border.color: "#333"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 0

            TabBar {
                id: rightTabs
                Layout.fillWidth: true
                TabButton { text: "Emotes" }
                TabButton { text: "Music" }
                TabButton { text: "Evidence" }
                TabButton { text: "Players" }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: rightTabs.currentIndex

                // ---- Emotes tab ----
                ColumnLayout {
                    spacing: 4

                    GridView {
                        id: emoteGrid
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        cellWidth: 48
                        cellHeight: 48
                        clip: true
                        model: uiBridge ? uiBridge.emotes : []

                        delegate: Rectangle {
                            required property int index
                            required property var modelData

                            width: emoteGrid.cellWidth - 4
                            height: emoteGrid.cellHeight - 4
                            color: uiBridge && uiBridge.selectedEmote === index
                                   ? "#3a3a5e"
                                   : emoteMA.containsMouse ? "#2a2a4e" : "#1e1e3e"
                            border.color: uiBridge && uiBridge.selectedEmote === index
                                          ? "#6a6aae" : "#333"
                            border.width: 1
                            radius: 3

                            Image {
                                id: emoteImg
                                anchors.fill: parent
                                anchors.margins: 2
                                source: "image://emoteicon/" + parent.index + "?g=" + (uiBridge ? uiBridge.emoteIconGeneration : 0)
                                sourceSize: Qt.size(40, 40)
                                visible: status === Image.Ready
                                cache: false
                                asynchronous: false
                            }

                            Label {
                                anchors.centerIn: parent
                                text: {
                                    var c = parent.modelData.comment
                                    return c ? c.substring(0, 4) : ""
                                }
                                visible: emoteImg.status !== Image.Ready
                                color: "#aaa"
                                font.pixelSize: 9
                            }

                            MouseArea {
                                id: emoteMA
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: { if (uiBridge) uiBridge.select_emote(parent.index) }
                            }

                            ToolTip.visible: emoteMA.containsMouse
                            ToolTip.text: modelData.comment || ""
                            ToolTip.delay: 400
                        }
                    }

                    CheckBox {
                        text: "Pre-animation"
                        checked: uiBridge ? uiBridge.preAnim : false
                        onToggled: { if (uiBridge) uiBridge.preAnim = checked }
                    }
                }

                // ---- Music tab ----
                ColumnLayout {
                    spacing: 4

                    TextField {
                        id: musicSearch
                        placeholderText: "Search..."
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: uiBridge && uiBridge.nowPlaying !== ""
                        text: "Now: " + (uiBridge ? uiBridge.nowPlaying : "")
                        color: "#80ccff"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: uiBridge ? uiBridge.musicTracks : []

                        delegate: Item {
                            id: trackDelegate
                            required property int index
                            required property var modelData

                            width: ListView.view ? ListView.view.width : 0
                            height: {
                                var search = musicSearch.text.toLowerCase()
                                if (search !== "" && !modelData.name.toLowerCase().includes(search))
                                    return 0
                                return modelData.isCategory ? 28 : 24
                            }
                            visible: height > 0

                            // Category separator
                            Rectangle {
                                anchors.fill: parent
                                visible: trackDelegate.modelData.isCategory
                                color: "transparent"
                                Rectangle {
                                    anchors.left: parent.left; anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.verticalCenterOffset: 4
                                    height: 1; color: "#444"
                                }
                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: trackDelegate.modelData.name
                                    font.bold: true; font.pixelSize: 11
                                    color: "#888"; leftPadding: 4
                                }
                            }

                            // Track entry
                            Rectangle {
                                anchors.fill: parent
                                visible: !trackDelegate.modelData.isCategory
                                color: trackMA.containsMouse ? "#2a2a4e" : "transparent"
                                radius: 2

                                Label {
                                    anchors.fill: parent
                                    text: trackDelegate.modelData.name
                                    color: trackMA.containsMouse ? "#fff" : "#ccc"
                                    font.pixelSize: 12; elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter; leftPadding: 8
                                }
                                MouseArea {
                                    id: trackMA; anchors.fill: parent; hoverEnabled: true
                                    onClicked: { if (uiBridge) uiBridge.play_music(trackDelegate.modelData.name) }
                                }
                            }
                        }
                    }
                }

                // ---- Evidence tab ----
                ColumnLayout {
                    spacing: 4

                    ListView {
                        id: evidenceList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: uiBridge ? uiBridge.evidence : []
                        currentIndex: -1

                        delegate: Rectangle {
                            id: eviDelegate
                            required property int index
                            required property var modelData

                            width: ListView.view ? ListView.view.width : 0
                            height: 24
                            color: evidenceList.currentIndex === eviDelegate.index
                                   ? "#3a3a5e" : eviMA.containsMouse ? "#2a2a4e" : "transparent"
                            radius: 2

                            Label {
                                anchors.fill: parent
                                text: eviDelegate.modelData.name || "(unnamed)"
                                color: "#ccc"; font.pixelSize: 12
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter; leftPadding: 8
                            }
                            MouseArea {
                                id: eviMA; anchors.fill: parent; hoverEnabled: true
                                onClicked: evidenceList.currentIndex = eviDelegate.index
                            }
                        }
                    }

                    // Evidence preview
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        color: "#12121e"; radius: 2
                        visible: evidenceList.currentIndex >= 0

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 4
                            Label {
                                property var item: {
                                    var i = evidenceList.currentIndex
                                    return (uiBridge && i >= 0 && i < uiBridge.evidence.length) ? uiBridge.evidence[i] : null
                                }
                                visible: item && item.image !== ""
                                text: item ? "Image: " + item.image : ""
                                color: "#666"; font.pixelSize: 11
                            }
                            Label {
                                property var item: {
                                    var i = evidenceList.currentIndex
                                    return (uiBridge && i >= 0 && i < uiBridge.evidence.length) ? uiBridge.evidence[i] : null
                                }
                                text: item ? item.description : ""
                                color: "#ccc"; font.pixelSize: 11
                                wrapMode: Text.WordWrap; Layout.fillWidth: true
                            }
                        }
                    }
                }

                // ---- Players tab ----
                ColumnLayout {
                    spacing: 4
                    Label {
                        text: {
                            var n = uiBridge ? uiBridge.players.length : 0
                            return n + " player" + (n !== 1 ? "s" : "") + " online"
                        }
                        color: "#aaa"; font.pixelSize: 12
                    }
                    ListView {
                        Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                        model: uiBridge ? uiBridge.players : []
                        delegate: Label {
                            required property var modelData
                            width: ListView.view ? ListView.view.width : 0
                            text: "\u2022 " + modelData
                            color: "#ccc"; font.pixelSize: 12; padding: 2; leftPadding: 8
                        }
                    }
                }
            }
        }
    }

    // ---- Bottom panel (IC controls + Chat) ----
    Rectangle {
        id: bottomPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: Math.max(root.height * 0.40, 280)
        color: "#0d0d18"

        RowLayout {
            anchors.fill: parent
            spacing: 2

            // IC controls
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                color: "#1a1a2e"
                border.color: "#333"; border.width: 1

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 6
                    contentHeight: icCol.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ColumnLayout {
                        id: icCol
                        width: parent.width
                        spacing: 4

                        // Health bars
                        RowLayout {
                            Layout.fillWidth: true; spacing: 4
                            Button {
                                visible: uiBridge && uiBridge.sideIndex === 3
                                text: "-"; implicitWidth: 24; implicitHeight: 20
                                onClicked: { if (uiBridge) uiBridge.set_health(1, uiBridge.defHp - 1) }
                            }
                            Label { text: "DEF"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 28 }
                            Rectangle {
                                Layout.fillWidth: true; height: 8; color: "#282828"; radius: 2
                                Rectangle {
                                    width: parent.width * (uiBridge ? uiBridge.defHp / 10.0 : 0)
                                    height: parent.height; color: "#50b450"; radius: 2
                                }
                            }
                            Button {
                                visible: uiBridge && uiBridge.sideIndex === 3
                                text: "+"; implicitWidth: 24; implicitHeight: 20
                                onClicked: { if (uiBridge) uiBridge.set_health(1, uiBridge.defHp + 1) }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 4
                            Button {
                                visible: uiBridge && uiBridge.sideIndex === 3
                                text: "-"; implicitWidth: 24; implicitHeight: 20
                                onClicked: { if (uiBridge) uiBridge.set_health(2, uiBridge.proHp - 1) }
                            }
                            Label { text: "PRO"; color: "#aaa"; font.pixelSize: 11; Layout.preferredWidth: 28 }
                            Rectangle {
                                Layout.fillWidth: true; height: 8; color: "#282828"; radius: 2
                                Rectangle {
                                    width: parent.width * (uiBridge ? uiBridge.proHp / 10.0 : 0)
                                    height: parent.height; color: "#c83c3c"; radius: 2
                                }
                            }
                            Button {
                                visible: uiBridge && uiBridge.sideIndex === 3
                                text: "+"; implicitWidth: 24; implicitHeight: 20
                                onClicked: { if (uiBridge) uiBridge.set_health(2, uiBridge.proHp + 1) }
                            }
                        }

                        // Timers
                        Label {
                            visible: uiBridge && uiBridge.timerDisplay !== ""
                            text: uiBridge ? uiBridge.timerDisplay : ""
                            color: "#aaa"; font.pixelSize: 11; font.family: "monospace"
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }

                        // Showname
                        RowLayout {
                            Layout.fillWidth: true; spacing: 4
                            Label { text: "Name:"; color: "#888"; font.pixelSize: 11 }
                            TextField {
                                id: shownameField
                                Layout.fillWidth: true; font.pixelSize: 12
                                text: uiBridge ? uiBridge.showname : ""
                                onTextEdited: { if (uiBridge) uiBridge.showname = text }
                            }
                        }

                        // IC message input
                        RowLayout {
                            Layout.fillWidth: true; spacing: 4
                            TextField {
                                id: icInput
                                placeholderText: "Type a message..."
                                Layout.fillWidth: true; font.pixelSize: 12
                                onAccepted: {
                                    if (uiBridge && text !== "") {
                                        uiBridge.send_ic_message(text)
                                        text = ""
                                    }
                                }
                            }
                            Button {
                                text: "Send"
                                onClicked: {
                                    if (uiBridge && icInput.text !== "") {
                                        uiBridge.send_ic_message(icInput.text)
                                        icInput.text = ""
                                        icInput.forceActiveFocus()
                                    }
                                }
                            }
                        }

                        // Interjections
                        RowLayout {
                            spacing: 4
                            Button {
                                text: "Hold It!"
                                highlighted: uiBridge && uiBridge.objectionMod === 1
                                onClicked: { if (uiBridge) uiBridge.objectionMod = (uiBridge.objectionMod === 1 ? 0 : 1) }
                            }
                            Button {
                                text: "Objection!"
                                highlighted: uiBridge && uiBridge.objectionMod === 2
                                onClicked: { if (uiBridge) uiBridge.objectionMod = (uiBridge.objectionMod === 2 ? 0 : 2) }
                            }
                            Button {
                                text: "Take That!"
                                highlighted: uiBridge && uiBridge.objectionMod === 3
                                onClicked: { if (uiBridge) uiBridge.objectionMod = (uiBridge.objectionMod === 3 ? 0 : 3) }
                            }
                        }

                        // Side + Color
                        RowLayout {
                            spacing: 4
                            Label { text: "Side:"; color: "#888"; font.pixelSize: 11 }
                            ComboBox {
                                model: ["Defense", "Prosecution", "Witness", "Judge", "Jury", "Seance", "Helper"]
                                currentIndex: uiBridge ? uiBridge.sideIndex : 0
                                onActivated: function(idx) { if (uiBridge) uiBridge.sideIndex = idx }
                                Layout.preferredWidth: 130
                            }
                            Label { text: "Color:"; color: "#888"; font.pixelSize: 11 }
                            ComboBox {
                                model: ["White", "Green", "Red", "Orange", "Blue", "Yellow", "Rainbow"]
                                currentIndex: uiBridge ? uiBridge.textColor : 0
                                onActivated: function(idx) { if (uiBridge) uiBridge.textColor = idx }
                                Layout.preferredWidth: 90
                            }
                        }

                        // Options
                        RowLayout {
                            spacing: 6
                            CheckBox {
                                text: "Flip"; checked: uiBridge ? uiBridge.flip : false
                                onToggled: { if (uiBridge) uiBridge.flip = checked }
                            }
                            CheckBox {
                                text: "Realization"; checked: uiBridge ? uiBridge.realizationEffect : false
                                onToggled: { if (uiBridge) uiBridge.realizationEffect = checked }
                            }
                            CheckBox {
                                text: "Screenshake"; checked: uiBridge ? uiBridge.screenshakeEffect : false
                                onToggled: { if (uiBridge) uiBridge.screenshakeEffect = checked }
                            }
                            CheckBox {
                                text: "Additive"; checked: uiBridge ? uiBridge.additiveText : false
                                onToggled: { if (uiBridge) uiBridge.additiveText = checked }
                            }
                        }

                        // Navigation
                        RowLayout {
                            spacing: 8
                            Button {
                                text: "Change Character"
                                onClicked: { if (uiBridge) uiBridge.pop_screen() }
                            }
                            Button {
                                text: "Disconnect"
                                onClicked: { if (uiBridge) uiBridge.request_disconnect() }
                            }
                        }
                    }
                }
            }

            // Chat panel
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                Layout.fillWidth: true
                color: "#1a1a2e"
                border.color: "#333"; border.width: 1

                ColumnLayout {
                    anchors.fill: parent; anchors.margins: 4; spacing: 0

                    TabBar {
                        id: chatTabs; Layout.fillWidth: true
                        TabButton { text: "IC Log" }
                        TabButton { text: "OOC Chat" }
                    }

                    StackLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        currentIndex: chatTabs.currentIndex

                        // IC Log
                        Rectangle {
                            color: "#12121e"
                            ScrollView {
                                id: icLogScroll; anchors.fill: parent
                                TextArea {
                                    text: uiBridge ? uiBridge.icLog : ""
                                    readOnly: true; wrapMode: TextEdit.Wrap
                                    color: "#ccc"; font.pixelSize: 12; background: null
                                    onTextChanged: {
                                        icLogScroll.ScrollBar.vertical.position =
                                            1.0 - icLogScroll.ScrollBar.vertical.size
                                    }
                                }
                            }
                        }

                        // OOC Chat
                        ColumnLayout {
                            spacing: 4
                            Rectangle {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                color: "#12121e"
                                ScrollView {
                                    id: oocLogScroll; anchors.fill: parent
                                    TextArea {
                                        text: uiBridge ? uiBridge.oocLog : ""
                                        readOnly: true; wrapMode: TextEdit.Wrap
                                        color: "#ccc"; font.pixelSize: 12; background: null
                                        onTextChanged: {
                                            oocLogScroll.ScrollBar.vertical.position =
                                                1.0 - oocLogScroll.ScrollBar.vertical.size
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true; spacing: 4; Layout.margins: 4
                                TextField {
                                    id: oocName; placeholderText: "Name"
                                    Layout.preferredWidth: 100; font.pixelSize: 12
                                }
                                TextField {
                                    id: oocInput; placeholderText: "OOC message..."
                                    Layout.fillWidth: true; font.pixelSize: 12
                                    onAccepted: {
                                        if (uiBridge && text !== "") {
                                            uiBridge.send_ooc_message(oocName.text, text)
                                            text = ""
                                        }
                                    }
                                }
                                Button {
                                    text: "Send"
                                    onClicked: {
                                        if (uiBridge && oocInput.text !== "") {
                                            uiBridge.send_ooc_message(oocName.text, oocInput.text)
                                            oocInput.text = ""
                                            oocInput.forceActiveFocus()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
