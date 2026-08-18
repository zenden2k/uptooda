import QtQuick
import QtQuick.Controls

Item {
    property string title
    property string description
    Column {
        anchors.centerIn: parent
        spacing: 8
        Label { anchors.horizontalCenter: parent.horizontalCenter; text: title; font.pixelSize: 21; color: "#44546a" }
        Label { anchors.horizontalCenter: parent.horizontalCenter; text: description; color: "#8996a6" }
    }
}
