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
    property color frameColor: "#004a6f"
    property color gradientColor1: "#0d567d"
    property color gradientColor2: "#06aeff"
    property color textColor: "#ffffff"
    property color strokeColor: "#000000"
    property int textBandHeight: 18
    property int addWidth: drawFrame ? frameWidth * 2 : 0
    property int addHeight: (drawFrame ? frameWidth * 2 : 0) + (drawText ? textBandHeight : 0)

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
        y: parent.height - root.textBandHeight - (root.drawFrame ? root.frameWidth : 0)
        width: parent.width
        height: root.textBandHeight + 1
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: root.gradientColor1 }
            GradientStop { position: 1.0; color: root.gradientColor2 }
        }
    }

    Text {
        visible: root.drawText
        x: 1
        y: parent.height - root.textBandHeight + 1 - (root.drawFrame ? root.frameWidth : 0)
        width: parent.width
        height: root.textBandHeight
        text: root.userText
        color: Qt.rgba(root.strokeColor.r, root.strokeColor.g, root.strokeColor.b, 0.65)
        font.family: "Tahoma"
        font.pointSize: 11
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Text {
        visible: root.drawText
        x: 0
        y: parent.height - root.textBandHeight - (root.drawFrame ? root.frameWidth : 0)
        width: parent.width
        height: root.textBandHeight
        text: root.userText
        color: root.textColor
        font.family: "Tahoma"
        font.pointSize: 11
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
