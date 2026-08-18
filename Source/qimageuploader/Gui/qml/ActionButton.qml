import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root
    hoverEnabled: true
    clip: true
    height: 44
    leftPadding: 10
    rightPadding: 10
    topPadding: 8
    bottomPadding: 8
    font.pixelSize: 14
    font.weight: Font.DemiBold
    contentItem: Text {
        text: root.text
        color: "#205574"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 7
        color: root.down ? "#75bce7" : root.hovered ? "#a9def8" : "#c5eafd"
        border.color: root.activeFocus ? "#489dd2" : "#9fd3ef"
        border.width: 1
    }
}
