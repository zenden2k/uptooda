import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: taskCard
    property string sessionId
    property int taskIndex
    property var taskData
    property real dragStartY: 0
    readonly property bool successfullyFinished: taskData.statusKey === "finished"
    readonly property real displayedProgress: successfullyFinished ? 100 : taskData.progress

    function statusColor(status) {
        switch (status) {
        case "finished":
            return "#278a56"
        case "failure":
            return "#c43d4b"
        case "stopped":
            return "#a54f55"
        case "running":
            return "#197db8"
        case "processing":
            return "#7760b5"
        case "postponed":
            return "#a36a16"
        default:
            return "#66768a"
        }
    }

    height: 84
    radius: 8
    color: dragArea.drag.active ? "#edf8ff" : taskHover.hovered ? "#f1f7fb" : "#f8fafc"
    border.color: dragArea.drag.active ? "#68b6e8" : taskHover.hovered ? "#b8d3e5" : "#dce4ec"
    border.width: 1
    z: dragArea.drag.active ? 20 : 0

    Behavior on color { ColorAnimation { duration: 100 } }

    HoverHandler {
        id: taskHover
    }

    Drag.active: dragArea.drag.active
    Drag.source: taskCard
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2

    DropArea {
        anchors.fill: parent
        onEntered: function(drag) {
            var source = drag.source
            if (source && source !== taskCard && source["sessionId"] === taskCard.sessionId) {
                uploadSessions.moveTask(taskCard.sessionId, source["taskIndex"], taskCard.taskIndex)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onDoubleClicked: mainWindowController.showCodes(taskCard.sessionId, taskCard.taskData.taskId)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 64
            Layout.preferredHeight: 64
            radius: 2
            color: "#eaf0f5"
            clip: true
            Image {
                anchors.fill: parent
                source: taskCard.taskData.thumbnail || "qrc:/res/images.ico"
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                sourceSize.width: 148
                sourceSize.height: 148
            }
            MouseArea {
                anchors.fill: parent
                enabled: taskCard.taskData.isImage === true
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: mainWindowController.openImageViewer(taskCard.sessionId, taskCard.taskData.taskId)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: taskCard.taskData.fileName
                    color: "#263548"
                    font.weight: Font.DemiBold
                    elide: Text.ElideMiddle
                    Layout.minimumWidth: 80
                    Layout.preferredWidth: Math.min(implicitWidth, 300)
                    Layout.maximumWidth: 300
                }
                Label {
                    text: qsTr("Server: %1").arg(taskCard.taskData.server)
                    color: "#607086"
                    elide: Text.ElideRight
                    Layout.maximumWidth: 230
                }
                Item { Layout.fillWidth: true }
            }

            RowLayout {
                Layout.fillWidth: true
                ProgressBar {
                    from: 0
                    to: 100
                    value: taskCard.displayedProgress
                    Layout.preferredWidth: 280
                    Layout.maximumWidth: 360
                    Layout.preferredHeight: 8
                    background: Rectangle { radius: 4; color: "#dfe8ef" }
                    contentItem: Item {
                        Rectangle {
                            width: parent.width * Math.max(0, Math.min(1, taskCard.displayedProgress / 100))
                            height: parent.height
                            radius: 4
                            color: "#5eb5e8"
                            Behavior on width {
                                enabled: !taskCard.successfullyFinished
                                NumberAnimation { duration: 120 }
                            }
                        }
                    }
                }
                Label { text: Math.round(taskCard.displayedProgress) + "%"; color: "#3f566d"; font.pixelSize: 12 }
                Label { text: taskCard.taskData.transferred; color: "#7c8999"; font.pixelSize: 11; Layout.preferredWidth: 116 }
                Item { Layout.fillWidth: true }
            }

            Label {
                text: taskCard.taskData.status
                color: taskCard.statusColor(taskCard.taskData.statusKey)
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
        }

        Button {
            id: codesButton
            text: qsTr("Codes")
            Layout.preferredWidth: 76
            Layout.preferredHeight: 34
            onClicked: mainWindowController.showCodes(taskCard.sessionId, taskCard.taskData.taskId)
            background: Rectangle { radius: 9; color: codesButton.hovered ? "#d8effd" : "#e9f6fd"; border.color: "#b8dcef" }
        }
        ToolButton {
            id: removeButton
            text: "×"
            font.pixelSize: 19
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            onClicked: mainWindowController.removeTask(taskCard.sessionId, taskCard.taskData.taskId)
            background: Rectangle { radius: 9; color: removeButton.hovered ? "#ffe4e7" : "#fff1f2" }
        }
    }

    MouseArea {
        id: dragArea
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 10
        cursorShape: Qt.SizeAllCursor
        drag.target: taskCard
        drag.axis: Drag.YAxis
        onPressed: taskCard.dragStartY = taskCard.y
        onReleased: taskCard.y = taskCard.dragStartY
    }
    TapHandler { acceptedButtons: Qt.RightButton; onTapped: taskMenu.popup() }

    Menu {
        id: taskMenu
        popupType: Popup.Item
        width: 218
        padding: 1
        spacing: 0
        AppMenuItem { text: qsTr("Codes"); onTriggered: mainWindowController.showCodes(taskCard.sessionId, taskCard.taskData.taskId) }
        AppMenuItem { text: qsTr("Copy direct link"); onTriggered: mainWindowController.copyDirectLink(taskCard.sessionId, taskCard.taskData.taskId) }
        AppMenuItem { text: qsTr("Copy view link"); onTriggered: mainWindowController.copyViewLink(taskCard.sessionId, taskCard.taskData.taskId) }
        AppMenuSeparator {}
        AppMenuItem { text: qsTr("Open in browser"); onTriggered: mainWindowController.openTaskUrl(taskCard.sessionId, taskCard.taskData.taskId) }
        AppMenuItem { text: qsTr("Open file"); onTriggered: mainWindowController.openTaskFile(taskCard.sessionId, taskCard.taskData.taskId) }
        AppMenuItem { text: qsTr("Copy file path"); onTriggered: mainWindowController.copyFilePath(taskCard.sessionId, taskCard.taskData.taskId) }
        AppMenuSeparator {}
        AppMenuItem { text: qsTr("Remove"); onTriggered: mainWindowController.removeTask(taskCard.sessionId, taskCard.taskData.taskId) }
        background: MenuBackground {}
    }
}
