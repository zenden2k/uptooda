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
    property color textColor: "#262626"
    property color strokeColor: "#ee000000"
    property int addWidth: 18
    property int addHeight: 33
    property int margin: 9
    property int labelHeight: 24

    Rectangle {
        anchors.fill: parent
        color: root.backgroundColor
    }

    Rectangle {
        x: 3
        y: 3
        width: Math.max(0, parent.width - 3)
        height: Math.max(0, parent.height - 3)
        color: "#26000000"
    }

    Rectangle {
        x: 1
        y: 1
        width: Math.max(0, parent.width - 3)
        height: Math.max(0, parent.height - 3)
        color: "#12000000"
    }

    Rectangle {
        id: frame
        x: 0
        y: 0
        width: Math.max(0, parent.width - 3)
        height: Math.max(0, parent.height - 3)
        color: "#f7f7f7"
        border.color: "#111111"
        border.width: 2
        antialiasing: true

        Rectangle {
            x: 3
            y: 3
            width: Math.max(0, parent.width - 6)
            height: Math.max(0, parent.height - 6)
            color: "transparent"
            border.color: "#ffffff"
            border.width: 1
        }

        Rectangle {
            x: 4
            y: 4
            width: Math.max(0, parent.width - 8)
            height: Math.max(0, parent.height - 8)
            color: "transparent"
            border.color: "#eeeeee"
            border.width: 1
        }
    }

    Image {
        x: root.margin
        y: root.margin
        width: root.thumbnailImageWidth
        height: root.thumbnailImageHeight
        source: "image://thumbnail/source"
        fillMode: Image.PreserveAspectCrop
        smooth: true
        clip: true
    }

    Rectangle {
        x: root.margin
        y: root.margin
        width: root.thumbnailImageWidth
        height: root.thumbnailImageHeight
        color: "transparent"
        border.color: "#fdfdfd"
        border.width: 1
    }

    Text {
        visible: root.drawText
        x: root.margin
        y: parent.height - root.labelHeight - 1
        width: Math.max(0, parent.width - 40)
        height: root.labelHeight
        text: root.userText
        color: root.strokeColor
        font.family: "Segoe UI"
        font.pointSize: 11
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Text {
        visible: root.drawText
        x: root.margin
        y: parent.height - root.labelHeight - 1
        width: Math.max(0, parent.width - 40)
        height: root.labelHeight
        text: root.userText
        color: root.textColor
        font.family: "Segoe UI"
        font.pointSize: 11
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Canvas {
        id: zoomIcon
        x: parent.width - 25
        y: parent.height - 22
        width: 17
        height: 17

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.lineCap = "round";
            ctx.lineJoin = "round";
            ctx.lineWidth = 2.2;

            ctx.strokeStyle = "#f9f9f9";
            ctx.beginPath();
            ctx.arc(6.4, 6.4, 4.1, 0, Math.PI * 2);
            ctx.moveTo(9.8, 9.8);
            ctx.lineTo(14.0, 14.0);
            ctx.stroke();

            ctx.strokeStyle = "#555555";
            ctx.beginPath();
            ctx.arc(6.0, 6.0, 4.0, 0, Math.PI * 2);
            ctx.moveTo(9.2, 9.2);
            ctx.lineTo(13.5, 13.5);
            ctx.stroke();
        }
    }
}
