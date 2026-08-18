import QtQuick
import QtQuick.Controls

MenuItem {
    id: control
    hoverEnabled: true
    implicitWidth: 190
    implicitHeight: 26
    width: parent ? parent.width : implicitWidth
    height: 26
    padding: 0
    horizontalPadding: 8
    verticalPadding: 0
    leftPadding: 8
    rightPadding: 8
    topPadding: 0
    bottomPadding: 0
    spacing: 0
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0

    contentItem: Text {
        text: control.text
        color: control.enabled ? "#263548" : "#9aa5b2"
        font: control.font
        leftPadding: 0
        rightPadding: 0
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 3
        color: control.highlighted ? "#e7f3fb" : "transparent"
    }
}
