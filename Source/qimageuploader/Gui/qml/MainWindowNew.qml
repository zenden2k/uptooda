import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    readonly property color pageColor: "#f3f6fa"
    readonly property color ink: "#263548"
    property int activeTab: 0
    property bool fileDropActive: false

    Connections {
        target: mainWindowController
        function onUploadSelectionRequested() { uploadDialog.open() }
        function onVideoImportRequested() { frameGrabberDialog.open() }
    }

    Rectangle { anchors.fill: parent; color: root.pageColor }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        MenuBar {
            id: menuBar
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            leftPadding: 8
            rightPadding: 8
            topPadding: 3
            bottomPadding: 3
            spacing: 1

            background: Rectangle {
                color: "#e9edf2"
                border.color: "#d7dde5"
            }

            delegate: MenuBarItem {
                id: menuBarItem
                implicitHeight: 34
                leftPadding: 11
                rightPadding: 11
                contentItem: Text {
                    text: menuBarItem.text.replace("&", "")
                    color: "#263548"
                    font: menuBarItem.font
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: menuBarItem.highlighted ? "#d9e1e9" : "transparent"
                }
            }

            Menu {
                id: fileMenu
                title: "&Файл"
                popupType: Popup.Item
                y: menuBar.height
                width: 150
                padding: 1
                spacing: 0
                AppMenuItem {
                    text: "Загрузить файлы…"
                    onTriggered: { fileMenu.close(); mainWindowController.chooseFiles() }
                }
                AppMenuSeparator {}
                AppMenuItem {
                    text: "Выход"
                    onTriggered: { fileMenu.close(); mainWindowController.quitApp() }
                }
                background: MenuBackground {}
            }
            Menu {
                id: toolsMenu
                title: "&Инструменты"
                popupType: Popup.Item
                y: menuBar.height
                width: 150
                padding: 1
                spacing: 0
                AppMenuItem {
                    text: "Сделать скриншот"
                    onTriggered: { toolsMenu.close(); mainWindowController.captureScreenshot() }
                }
                AppMenuItem {
                    text: "Показать журнал"
                    onTriggered: { toolsMenu.close(); mainWindowController.showLog() }
                }
                background: MenuBackground {}
            }
            Menu {
                id: helpMenu
                title: "&Справка"
                popupType: Popup.Item
                y: menuBar.height
                width: 150
                padding: 1
                spacing: 0
                AppMenuItem {
                    text: "О программе"
                    onTriggered: { helpMenu.close(); mainWindowController.showAbout() }
                }
                background: MenuBackground {}
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#f9fbfd"
            border.color: "#e1e7ee"

            RowLayout {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 12

                ActionButton {
                    text: "＋  Загрузить файлы"
                    onClicked: mainWindowController.chooseFiles()
                }
                ActionButton {
                    text: "▣  Сделать скриншот"
                    onClicked: mainWindowController.captureScreenshot()
                }
                ActionButton {
                    text: "▶  Импорт видеофайла"
                    onClicked: mainWindowController.importVideo()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "#ffffff"
            border.color: "#e2e8ef"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 22
                anchors.bottom: parent.bottom
                height: parent.height
                spacing: 8

                Repeater {
                    model: ["Загрузки", "Скриншоты", "Записи экрана"]
                    delegate: AbstractButton {
                        id: tabButton
                        required property string modelData
                        required property int index
                        hoverEnabled: true
                        text: modelData
                        height: parent.height
                        leftPadding: 18
                        rightPadding: 18
                        font.weight: root.activeTab === index ? Font.DemiBold : Font.Normal
                        onClicked: root.activeTab = index
                        contentItem: Text {
                            text: tabButton.text
                            color: root.activeTab === index ? "#2789c9" : "#5b697b"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Item {
                            Rectangle {
                                anchors.fill: parent
                                anchors.bottomMargin: 3
                                color: tabButton.hovered ? "#edf6fc" : "transparent"
                            }
                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 3
                                color: root.activeTab === index ? "#55afe8" : "transparent"
                            }
                        }
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.activeTab

            Item {
                ListView {
                    id: sessionsView
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12
                    clip: true
                    model: uploadSessions
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: UploadSessionCard {
                        width: sessionsView.width - (sessionsView.ScrollBar.vertical.visible ? 14 : 0)
                    }

                    footer: Item { width: 1; height: 12 }
                    Label {
                        anchors.centerIn: parent
                        visible: sessionsView.count === 0
                        text: "Здесь появятся ваши загрузки"
                        color: "#8996a6"
                        font.pixelSize: 16
                    }
                }
            }

            EmptyTab { title: "Скриншоты"; description: "Раздел будет подключён к истории снимков." }
            EmptyTab { title: "Записи экрана"; description: "Раздел будет подключён к записям экрана." }
        }
    }

    DropArea {
        anchors.fill: parent
        enabled: !uploadDialog.visible && !frameGrabberDialog.visible
        onEntered: function(drag) {
            var containsLocalFile = false
            for (var i = 0; i < drag.urls.length; ++i) {
                if (drag.urls[i].toString().startsWith("file:")) {
                    containsLocalFile = true
                    break
                }
            }
            drag.accepted = containsLocalFile
            root.fileDropActive = containsLocalFile
        }
        onExited: root.fileDropActive = false
        onDropped: function(drop) {
            root.fileDropActive = false
            mainWindowController.addDroppedFiles(drop.urls)
            drop.acceptProposedAction()
        }
    }

    FrameGrabberDialog {
        id: frameGrabberDialog
        controller: frameGrabberController
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 8
        z: 100
        visible: root.fileDropActive
        radius: 10
        color: "#e8f6ff"
        opacity: 0.94
        border.color: "#55afe8"
        border.width: 2

        Column {
            anchors.centerIn: parent
            spacing: 8
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Отпустите файлы для загрузки"
                color: "#257aaf"
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "После этого можно будет выбрать серверы"
                color: "#607f94"
                font.pixelSize: 13
            }
        }
    }

    Dialog {
        id: uploadDialog

        function confirmSelection() {
            if (!uploadButton.enabled)
                return
            mainWindowController.confirmUpload(imageSelector.selectedServer, imageSelector.selectedAccount,
                                               fileSelector.selectedServer, fileSelector.selectedAccount)
            uploadDialog.close()
        }

        function cancelSelection() {
            mainWindowController.cancelUpload()
            uploadDialog.close()
        }

        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 720)
        modal: true
        closePolicy: Popup.NoAutoClose
        padding: 0

        Shortcut {
            sequences: ["Return", "Enter"]
            context: Qt.WindowShortcut
            enabled: uploadDialog.visible && uploadButton.enabled
            onActivated: uploadDialog.confirmSelection()
        }
        Shortcut {
            sequence: "Escape"
            context: Qt.WindowShortcut
            enabled: uploadDialog.visible
            onActivated: uploadDialog.cancelSelection()
        }

        background: Rectangle {
            radius: 18
            color: "#ffffff"
            border.color: "#cad7e4"
        }
        contentItem: ColumnLayout {
            spacing: 14
            anchors.margins: 22

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 14
                Layout.leftMargin: 22
                Layout.rightMargin: 14

                Label {
                    text: "Параметры загрузки"
                    color: root.ink
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }

                AbstractButton {
                    id: closeUploadDialogButton
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    hoverEnabled: true
                    ToolTip.visible: hovered
                    ToolTip.text: "Закрыть"
                    ToolTip.delay: 600
                    onClicked: uploadDialog.cancelSelection()
                    contentItem: Canvas {
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onPaint: {
                            const context = getContext("2d")
                            const centerX = width / 2
                            const centerY = height / 2
                            context.clearRect(0, 0, width, height)
                            context.strokeStyle = "#526273"
                            context.lineWidth = 1.8
                            context.lineCap = "round"
                            context.beginPath()
                            context.moveTo(centerX - 5, centerY - 5)
                            context.lineTo(centerX + 5, centerY + 5)
                            context.moveTo(centerX + 5, centerY - 5)
                            context.lineTo(centerX - 5, centerY + 5)
                            context.stroke()
                        }
                    }
                    background: Rectangle {
                        radius: 8
                        color: "#e8edf3"
                        opacity: closeUploadDialogButton.hovered ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 100 } }
                    }
                }
            }
            Label {
                text: "Выбрано файлов: " + mainWindowController.pendingFileCount
                color: "#6a7889"
                Layout.leftMargin: 22
            }
            ServerSelector {
                id: imageSelector
                title: "Сервер для изображений"
                serverModel: mainWindowController.imageServers
                initialServer: mainWindowController.defaultImageServer
                initialAccount: mainWindowController.defaultImageAccount
                Layout.fillWidth: true
                Layout.leftMargin: 22
                Layout.rightMargin: 22
            }
            ServerSelector {
                id: fileSelector
                title: "Сервер для остальных файлов"
                serverModel: mainWindowController.fileServers
                initialServer: mainWindowController.defaultFileServer
                initialAccount: mainWindowController.defaultFileAccount
                Layout.fillWidth: true
                Layout.leftMargin: 22
                Layout.rightMargin: 22
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 22
                Layout.rightMargin: 22
                Layout.bottomMargin: 22
                Item { Layout.fillWidth: true }
                AbstractButton {
                    id: cancelButton
                    text: "Отмена"
                    hoverEnabled: true
                    leftPadding: 20
                    rightPadding: 20
                    topPadding: 10
                    bottomPadding: 10
                    onClicked: uploadDialog.cancelSelection()
                    contentItem: Text {
                        text: cancelButton.text
                        color: "#3f4d5e"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 10; color: cancelButton.hovered ? "#e8edf3" : "#f1f4f7" }
                }
                AbstractButton {
                    id: uploadButton
                    text: "Загрузить"
                    hoverEnabled: true
                    leftPadding: 20
                    rightPadding: 20
                    topPadding: 10
                    bottomPadding: 10
                    enabled: imageSelector.selectedServer.length > 0 && fileSelector.selectedServer.length > 0
                    onClicked: uploadDialog.confirmSelection()
                    contentItem: Text {
                        text: uploadButton.text
                        color: uploadButton.enabled ? "#205574" : "#8a98a7"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { radius: 10; color: uploadButton.hovered ? "#75c2f0" : "#98d6f7" }
                }
            }
        }
    }
}
