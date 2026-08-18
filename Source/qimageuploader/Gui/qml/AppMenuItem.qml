import QtQuick
import QtQuick.Controls

MenuItem {
    id: control
    hoverEnabled: true
    implicitWidth: 190
    implicitHeight: 24
    padding: 0
    leftPadding: 6
    rightPadding: 6
    spacing: 0
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0

    contentItem: Text {
        text: control.text
        color: control.enabled ? "#263548" : "#9aa5b2"
        font: control.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 4
        color: control.highlighted ? "#e7f3fb" : "transparent"
    }
}
