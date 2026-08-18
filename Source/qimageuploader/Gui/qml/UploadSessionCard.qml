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
    property bool expanded: fileCount <= 3

    implicitHeight: header.height + (expanded ? taskColumn.implicitHeight + 12 : 0)
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
        height: 66

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 12

            Image { source: card.serverIcon; Layout.preferredWidth: 30; Layout.preferredHeight: 30; fillMode: Image.PreserveAspectFit }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    Layout.fillWidth: true
                    text: card.serverNames.length > 0 ? card.serverNames : "Сессия загрузки"
                    color: "#263548"
                    font.weight: Font.DemiBold
                    font.pixelSize: 14
                    elide: Text.ElideRight
                }
                Label { text: card.fileCount + " " + (card.fileCount === 1 ? "файл" : "файлов"); color: "#7c8999"; font.pixelSize: 12 }
            }
            ToolButton {
                id: expandButton
                text: card.expanded ? "⌃" : "⌄"
                visible: card.fileCount > 3
                onClicked: card.expanded = !card.expanded
                background: Rectangle { radius: 9; color: expandButton.hovered ? "#eaf3fa" : "transparent" }
            }
            ToolButton {
                id: sessionMenuButton
                text: "⋮"
                onClicked: sessionMenu.popup()
                background: Rectangle { radius: 9; color: sessionMenuButton.hovered ? "#eaf3fa" : "transparent" }
            }
        }
        TapHandler { onTapped: if (card.fileCount > 3) card.expanded = !card.expanded }
        TapHandler { acceptedButtons: Qt.RightButton; onTapped: sessionMenu.popup() }
    }

    Column {
        id: taskColumn
        x: 12
        y: header.height
        width: parent.width - 24
        spacing: 8
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
        AppMenuItem { text: "Коды"; onTriggered: mainWindowController.showCodes(card.sessionId) }
        AppMenuItem { text: "Повторить неудачные"; onTriggered: mainWindowController.retrySession(card.sessionId) }
        AppMenuSeparator {}
        AppMenuItem { text: "Удалить сессию"; onTriggered: mainWindowController.removeSession(card.sessionId) }
        background: MenuBackground {}
    }
}
