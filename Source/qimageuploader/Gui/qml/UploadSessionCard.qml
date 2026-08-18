import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: card
    required property string sessionId
    required property int fileCount
    required property string serverNames
    required property string serverIcon
    required property var tasks
    property bool expanded: true

    implicitHeight: header.height + (expanded ? taskColumn.implicitHeight + 8 : 0)
    height: implicitHeight
    radius: 9
    color: "#ffffff"
    border.color: "#d8e1ea"
    border.width: 1
    clip: true

    Behavior on height { NumberAnimation { duration: 210; easing.type: Easing.OutCubic } }

    Item {
        id: header
        width: parent.width
        height: 58

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 10
            spacing: 10

            Image { source: card.serverIcon; Layout.preferredWidth: 30; Layout.preferredHeight: 30; fillMode: Image.PreserveAspectFit }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    Layout.fillWidth: true
                    text: card.serverNames.length > 0 ? card.serverNames : qsTr("Upload session")
                    color: "#263548"
                    font.weight: Font.DemiBold
                    font.pixelSize: 14
                    elide: Text.ElideRight
                }
                Label { text: qsTr("%n file(s)", "", card.fileCount); color: "#7c8999"; font.pixelSize: 12 }
            }
            ToolButton {
                id: sessionMenuButton
                text: "⋮"
                onClicked: sessionMenu.popup()
                background: Rectangle { radius: 9; color: sessionMenuButton.hovered ? "#eaf3fa" : "transparent" }
            }
        }
        TapHandler { acceptedButtons: Qt.RightButton; onTapped: sessionMenu.popup() }
    }

    Column {
        id: taskColumn
        x: 8
        y: header.height
        width: parent.width - 16
        spacing: 6
        visible: card.expanded

        Repeater {
            model: card.tasks
            delegate: UploadTaskCard {
                required property var modelData
                required property int index
                width: taskColumn.width
                sessionId: card.sessionId
                taskIndex: index
                taskData: modelData
            }
        }
    }

    Menu {
        id: sessionMenu
        popupType: Popup.Item
        width: 198
        padding: 1
        spacing: 0
        AppMenuItem { text: qsTr("Codes"); onTriggered: mainWindowController.showCodes(card.sessionId) }
        AppMenuItem { text: qsTr("Retry failed"); onTriggered: mainWindowController.retrySession(card.sessionId) }
        AppMenuSeparator {}
        AppMenuItem { text: qsTr("Remove session"); onTriggered: mainWindowController.removeSession(card.sessionId) }
        background: MenuBackground {}
    }
}
