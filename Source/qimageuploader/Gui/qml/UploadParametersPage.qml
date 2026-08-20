import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var controller

    signal uploadStarted()
    signal backRequested()

    function confirmSelection() {
        if (!uploadButton.enabled)
            return
        if (root.controller.confirmUpload(imageSelector.selectedServer, imageSelector.selectedAccount,
                                          fileSelector.selectedServer, fileSelector.selectedAccount))
            root.uploadStarted()
    }

    Shortcut {
        sequences: ["Return", "Enter"]
        context: Qt.WindowShortcut
        enabled: root.controller.shortcutScopeActive && root.visible && uploadButton.enabled
        onActivated: root.confirmSelection()
    }

    Shortcut {
        sequence: "Escape"
        context: Qt.WindowShortcut
        enabled: root.controller.shortcutScopeActive && root.visible
        onActivated: root.backRequested()
    }

    ColumnLayout {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 28
        width: Math.min(parent.width - 48, 720)
        spacing: 14

        Label {
            text: qsTr("Upload settings")
            color: "#263548"
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }

        Label {
            text: qsTr("Selected files: %1").arg(root.controller.pendingFileCount)
            color: "#6a7889"
        }

        ServerSelector {
            id: imageSelector
            title: qsTr("Image server")
            serverModel: root.controller.imageServers
            initialServer: root.controller.defaultImageServer
            initialAccount: root.controller.defaultImageAccount
            Layout.fillWidth: true
        }

        ServerSelector {
            id: fileSelector
            title: qsTr("Server for other files")
            serverModel: root.controller.fileServers
            initialServer: root.controller.defaultFileServer
            initialAccount: root.controller.defaultFileAccount
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }

            AbstractButton {
                id: backButton
                text: qsTr("< Back")
                hoverEnabled: true
                leftPadding: 20
                rightPadding: 20
                topPadding: 10
                bottomPadding: 10
                onClicked: root.backRequested()
                contentItem: Text {
                    text: backButton.text
                    color: "#3f4d5e"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 10
                    color: backButton.hovered ? "#e8edf3" : "#f1f4f7"
                }
            }

            AbstractButton {
                id: uploadButton
                text: qsTr("Upload")
                hoverEnabled: true
                leftPadding: 20
                rightPadding: 20
                topPadding: 10
                bottomPadding: 10
                enabled: root.controller.pendingFileCount > 0
                         && imageSelector.selectedServer.length > 0
                         && fileSelector.selectedServer.length > 0
                onClicked: root.confirmSelection()
                contentItem: Text {
                    text: uploadButton.text
                    color: uploadButton.enabled ? "#205574" : "#8a98a7"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 10
                    color: uploadButton.hovered && uploadButton.enabled ? "#75c2f0" : "#98d6f7"
                }
            }
        }
    }
}
