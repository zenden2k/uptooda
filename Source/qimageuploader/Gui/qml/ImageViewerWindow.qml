import QtQuick
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent
    focus: true

    Keys.onLeftPressed: imageViewerController.showPrevious()
    Keys.onRightPressed: imageViewerController.showNext()
    Keys.onEscapePressed: imageViewerController.closeViewer()

    Rectangle {
        anchors.fill: parent
        color: "#b5121519"
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onClicked: imageViewerController.closeViewer()
    }

    Image {
        id: image
        anchors.centerIn: parent
        width: Math.min(implicitWidth, Math.max(0, parent.width - 184))
        height: Math.min(implicitHeight, Math.max(0, parent.height - 130))
        source: imageViewerController.imageSource
        fillMode: Image.PreserveAspectFit
        asynchronous: false
        cache: false
        smooth: true
        mipmap: true

        MouseArea {
            anchors.centerIn: parent
            width: image.paintedWidth
            height: image.paintedHeight
            acceptedButtons: Qt.LeftButton
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: imageViewerController.loading
        visible: running
    }

    Label {
        anchors.centerIn: parent
        visible: !imageViewerController.loading && imageViewerController.errorText.length > 0
        text: imageViewerController.errorText
        color: "#f0f2f4"
        font.pixelSize: 16
    }

    AbstractButton {
        id: previousButton
        readonly property bool navigationAvailable: imageViewerController.hasPrevious
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        width: 54
        height: 86
        hoverEnabled: true
        onClicked: {
            if (navigationAvailable)
                imageViewerController.showPrevious()
        }
        contentItem: Canvas {
            opacity: previousButton.navigationAvailable ? 1.0 : 0.35
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const context = getContext("2d")
                context.clearRect(0, 0, width, height)
                context.strokeStyle = "#ffffff"
                context.lineWidth = 3
                context.lineCap = "round"
                context.lineJoin = "round"
                context.beginPath()
                context.moveTo(width * 0.62, height * 0.34)
                context.lineTo(width * 0.42, height * 0.50)
                context.lineTo(width * 0.62, height * 0.66)
                context.stroke()
            }
        }
        background: Rectangle {
            radius: 8
            color: previousButton.hovered && previousButton.navigationAvailable ? "#4f626a73" : "#26323840"
        }
    }

    AbstractButton {
        id: nextButton
        readonly property bool navigationAvailable: imageViewerController.hasNext
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        width: 54
        height: 86
        hoverEnabled: true
        onClicked: {
            if (navigationAvailable)
                imageViewerController.showNext()
        }
        contentItem: Canvas {
            opacity: nextButton.navigationAvailable ? 1.0 : 0.35
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const context = getContext("2d")
                context.clearRect(0, 0, width, height)
                context.strokeStyle = "#ffffff"
                context.lineWidth = 3
                context.lineCap = "round"
                context.lineJoin = "round"
                context.beginPath()
                context.moveTo(width * 0.38, height * 0.34)
                context.lineTo(width * 0.58, height * 0.50)
                context.lineTo(width * 0.38, height * 0.66)
                context.stroke()
            }
        }
        background: Rectangle {
            radius: 8
            color: nextButton.hovered && nextButton.navigationAvailable ? "#4f626a73" : "#26323840"
        }
    }

    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 12
        spacing: 4

        Repeater {
            model: [
                { "tip": qsTr("Minimize") },
                { "tip": qsTr("Full screen") },
                { "tip": qsTr("Close") }
            ]
            delegate: AbstractButton {
                id: windowButton
                required property var modelData
                required property int index
                width: 42
                height: 36
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: modelData.tip
                ToolTip.delay: 600
                onClicked: {
                    if (index === 0)
                        imageViewerController.minimizeViewer()
                    else if (index === 1)
                        imageViewerController.toggleFullScreen()
                    else
                        imageViewerController.closeViewer()
                }
                contentItem: Canvas {
                    opacity: windowButton.hovered ? 1.0 : 0.82
                    Behavior on opacity { NumberAnimation { duration: 100 } }
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    onPaint: {
                        const context = getContext("2d")
                        const centerX = width / 2
                        const centerY = height / 2
                        context.clearRect(0, 0, width, height)
                        context.strokeStyle = "#ffffff"
                        context.lineWidth = 1.7
                        context.lineCap = "round"
                        context.lineJoin = "round"
                        context.beginPath()
                        if (windowButton.index === 0) {
                            context.moveTo(centerX - 6, centerY)
                            context.lineTo(centerX + 6, centerY)
                        } else if (windowButton.index === 1) {
                            context.rect(centerX - 6, centerY - 5, 12, 10)
                        } else {
                            context.moveTo(centerX - 5, centerY - 5)
                            context.lineTo(centerX + 5, centerY + 5)
                            context.moveTo(centerX + 5, centerY - 5)
                            context.lineTo(centerX - 5, centerY + 5)
                        }
                        context.stroke()
                    }
                }
                background: Rectangle {
                    radius: 5
                    opacity: windowButton.hovered ? 0.92 : 0.76
                    color: windowButton.hovered
                           ? (windowButton.index === 2 ? "#d94b55" : "#53616d")
                           : "#303b45"
                    Behavior on opacity { NumberAnimation { duration: 100 } }
                }
            }
        }
    }

    Label {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 22
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(implicitWidth, parent.width - 180)
        text: imageViewerController.fileName
        color: "#f2f4f6"
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideMiddle
    }
}
