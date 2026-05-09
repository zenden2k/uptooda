import QtQuick

Item {
    id: root

    property int sourceWidth: 0
    property int sourceHeight: 0
    property int thumbnailImageWidth: 0
    property int thumbnailImageHeight: 0
    property string userText: ""
    property bool drawText: true
    property bool drawFrame: true
    property color backgroundColor: "transparent"
    property int frameWidth: 1
    property color frameColor: "#005500"
    property color gradientColor1: "#0d567d"
    property color gradientColor2: "#0656ff"
    property color textColor: "#ffffff"
    property color strokeColor: "#000000"
    property int addWidth: drawFrame ? frameWidth * 2 : 0
    property int addHeight: drawFrame ? frameWidth * 2 : 0

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
    }

    Image {
        x: root.drawFrame ? root.frameWidth : 0
        y: root.drawFrame ? root.frameWidth : 0
        width: root.thumbnailImageWidth
        height: root.thumbnailImageHeight
        source: "image://thumbnail/source"
        fillMode: Image.PreserveAspectCrop
        smooth: true
    }

    Rectangle {
        visible: root.drawText
        x: 0
        y: parent.height - textLabel.implicitHeight - 3 - (root.drawFrame ? root.frameWidth : 0)
        width: parent.width
        height: textLabel.implicitHeight + 4
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Qt.rgba(root.gradientColor1.r, root.gradientColor1.g, root.gradientColor1.b, 0.53) }
            GradientStop { position: 1.0; color: Qt.rgba(root.gradientColor2.r, root.gradientColor2.g, root.gradientColor2.b, 0.53) }
        }
    }

    Text {
        id: textShadow
        visible: root.drawText
        anchors.fill: textLabel
        anchors.leftMargin: 1
        anchors.topMargin: 1
        text: root.userText
        color: Qt.rgba(root.strokeColor.r, root.strokeColor.g, root.strokeColor.b, 0.65)
        font.family: "Tahoma"
        font.pointSize: 12
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Text {
        id: textLabel
        visible: root.drawText
        x: 0
        y: parent.height - implicitHeight - 2 - (root.drawFrame ? root.frameWidth : 0)
        width: parent.width
        height: implicitHeight + 1
        text: root.userText
        color: root.textColor
        font.family: "Tahoma"
        font.pointSize: 12
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Rectangle {
        visible: root.drawFrame
        anchors.fill: parent
        color: "transparent"
        border.color: root.frameColor
        border.width: root.frameWidth
    }
}
