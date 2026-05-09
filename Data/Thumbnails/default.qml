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
    property color textColor: "#ffffff"
    property color strokeColor: "#000000"
    property int addWidth: 4
    property int addHeight: 19

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
    }

    Image {
        x: 2
        y: 2
        width: root.thumbnailImageWidth
        height: root.thumbnailImageHeight
        source: "image://thumbnail/source"
        fillMode: Image.PreserveAspectCrop
        smooth: true
    }

    BorderImage {
        anchors.fill: parent
        source: "default.png"
        border.left: 6
        border.top: 12
        border.right: 7
        border.bottom: 17
        horizontalTileMode: BorderImage.Stretch
        verticalTileMode: BorderImage.Stretch
    }

    Text {
        visible: root.drawText
        x: 31
        y: parent.height - 16
        width: parent.width - 34
        height: 17
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
        x: 30
        y: parent.height - 17
        width: parent.width - 32
        height: 17
        text: root.userText
        color: Qt.rgba(root.textColor.r, root.textColor.g, root.textColor.b, 0.17)
        font.family: "Tahoma"
        font.pointSize: 11
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
