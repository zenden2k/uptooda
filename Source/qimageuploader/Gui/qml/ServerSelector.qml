import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property string title: qsTr("Server")
    property var serverModel: []
    property string initialServer: ""
    property string initialAccount: ""
    property string selectedServer: ""
    property string selectedAccount: ""
    property var accountModel: []
    property bool initialized: false

    signal addAccountRequested(string serverName)

    implicitHeight: 116
    radius: 14
    color: "#f8fbff"
    border.color: "#cbdced"
    border.width: 1

    function selectServer(serverName, accountName) {
        var found = 0
        for (var i = 0; i < serverModel.length; ++i) {
            if (serverModel[i].name === serverName) {
                found = i
                break
            }
        }
        serverBox.currentIndex = found
        updateAccounts(accountName)
        initialized = true
    }

    function updateAccounts(accountName) {
        selectedServer = serverBox.currentIndex >= 0 && serverModel.length > serverBox.currentIndex
                         ? serverModel[serverBox.currentIndex].name : ""
        accountModel = serverBox.currentIndex >= 0 && serverModel.length > serverBox.currentIndex
                       ? serverModel[serverBox.currentIndex].accounts : []
        accountBox.currentIndex = 0
        selectAccount(accountName || "")
    }

    function selectAccount(accountName) {
        for (var i = 0; i < accountModel.length; ++i) {
            if (accountModel[i].name === accountName) {
                accountBox.currentIndex = i
                selectedAccount = accountModel[i].name
                return
            }
        }
        selectedAccount = accountBox.currentIndex >= 0 && accountModel.length > accountBox.currentIndex
                          ? accountModel[accountBox.currentIndex].name : ""
    }

    onServerModelChanged: {
        if (initialized) {
            selectServer(selectedServer, selectedAccount)
        }
    }
    Component.onCompleted: selectServer(initialServer, initialAccount)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        Label {
            text: root.title
            color: "#22324a"
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ComboBox {
                id: serverBox
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                model: root.serverModel
                textRole: "displayName"
                valueRole: "name"
                onActivated: root.updateAccounts("")
                leftPadding: 42
                rightPadding: 42

                contentItem: Text {
                    leftPadding: 0
                    text: serverBox.displayText
                    color: "#1f2c3d"
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 10
                    color: serverBox.hovered ? "#ffffff" : "#f1f6fb"
                    border.color: serverBox.activeFocus ? "#65aee8" : "#c6d5e3"
                }
                indicator: Item {
                    x: serverBox.width - width - 7
                    anchors.verticalCenter: parent.verticalCenter
                    width: 30
                    height: 30
                    rotation: serverBox.popup.visible ? 180 : 0

                    Behavior on rotation {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Canvas {
                        id: serverChevron
                        readonly property color strokeColor: serverBox.popup.visible ? "#287caf" : "#58708b"
                        anchors.centerIn: parent
                        width: 14
                        height: 9
                        onStrokeColorChanged: requestPaint()
                        onPaint: {
                            const context = getContext("2d")
                            context.clearRect(0, 0, width, height)
                            context.strokeStyle = strokeColor
                            context.lineWidth = 2
                            context.lineCap = "round"
                            context.lineJoin = "round"
                            context.beginPath()
                            context.moveTo(1.5, 2)
                            context.lineTo(width / 2, height - 2)
                            context.lineTo(width - 1.5, 2)
                            context.stroke()
                        }
                    }
                }
                Image {
                    x: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20
                    height: 20
                    source: serverBox.currentIndex >= 0 && root.serverModel.length > serverBox.currentIndex
                            ? root.serverModel[serverBox.currentIndex].icon : "qrc:/res/server.png"
                    fillMode: Image.PreserveAspectFit
                }
                popup: Popup {
                    y: serverBox.height + 5
                    width: serverBox.width
                    implicitHeight: Math.min(contentItem.implicitHeight + 12, 280)
                    padding: 6
                    background: Rectangle {
                        radius: 12
                        color: "#ffffff"
                        border.color: "#c8d7e5"
                    }
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: serverBox.popup.visible ? serverBox.delegateModel : null
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                }
                delegate: ItemDelegate {
                    width: serverBox.width - 12
                    height: 38
                    highlighted: serverBox.highlightedIndex === index
                    contentItem: RowLayout {
                        Image { source: modelData.icon; Layout.preferredWidth: 20; Layout.preferredHeight: 20 }
                        Text { text: modelData.displayName; color: "#24354a"; elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                    background: Rectangle { radius: 8; color: highlighted ? "#e5f3ff" : "transparent" }
                }
            }

            ComboBox {
                id: accountBox
                Layout.preferredWidth: 210
                Layout.preferredHeight: 42
                model: root.accountModel
                textRole: "displayName"
                valueRole: "name"
                onActivated: root.selectAccount(accountBox.currentValue)
                background: Rectangle {
                    radius: 10
                    color: accountBox.hovered ? "#ffffff" : "#f1f6fb"
                    border.color: accountBox.activeFocus ? "#65aee8" : "#c6d5e3"
                }
                popup: Popup {
                    y: accountBox.height + 5
                    width: accountBox.width
                    implicitHeight: Math.min(contentItem.implicitHeight + 12, 280)
                    padding: 6
                    background: Rectangle {
                        radius: 12
                        color: "#ffffff"
                        border.color: "#c8d7e5"
                    }
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: accountBox.popup.visible ? accountBox.delegateModel : null
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                }
                delegate: ItemDelegate {
                    width: accountBox.width - 12
                    height: 38
                    highlighted: accountBox.highlightedIndex === index
                    contentItem: RowLayout {
                        spacing: 8
                        Rectangle {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            radius: 10
                            color: "#dcecf7"
                            Text {
                                anchors.centerIn: parent
                                text: modelData.name.length > 0 ? modelData.displayName.charAt(0).toUpperCase() : "−"
                                color: "#426783"
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }
                        }
                        Text {
                            text: modelData.displayName
                            color: "#24354a"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    background: Rectangle { radius: 8; color: highlighted ? "#e5f3ff" : "transparent" }
                }
            }

            AbstractButton {
                id: addAccountButton
                text: "+"
                hoverEnabled: true
                Layout.preferredWidth: 42
                Layout.preferredHeight: 42
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Add account")
                ToolTip.delay: 600
                onClicked: {
                    var server = root.selectedServer
                    var account = mainWindowController.addAccount(server)
                    if (account.length > 0) {
                        root.selectServer(server, account)
                    }
                }
                contentItem: Text {
                    text: addAccountButton.text
                    color: "#315e7b"
                    font.pixelSize: 18
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 10
                    color: addAccountButton.hovered ? "#d9efff" : "#eaf6ff"
                    border.color: "#afd7f3"
                }
            }
        }
    }
}
