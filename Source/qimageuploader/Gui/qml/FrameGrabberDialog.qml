pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    required property var controller

    function cancelDialog() {
        controller.cancel()
        close()
    }

    function acceptDialog() {
        if (!controller.canAccept)
            return
        close()
        controller.acceptFrames()
    }

    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 920)
    height: Math.min(parent.height - 48, 620)
    modal: true
    closePolicy: Popup.NoAutoClose
    padding: 0

    Shortcut {
        sequence: "Escape"
        context: Qt.WindowShortcut
        enabled: dialog.visible
        onActivated: dialog.cancelDialog()
    }

    onOpened: thumbsView.clearSelection()

    background: Rectangle {
        radius: 16
        color: "#ffffff"
        border.color: "#cad7e4"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 14

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.topMargin: 14
            Layout.leftMargin: 20
            Layout.rightMargin: 14

            ColumnLayout {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Label {
                    text: "Импорт видеофайла"
                    color: "#263548"
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                }
                Label {
                    text: "Извлечение равномерно распределённых кадров"
                    color: "#748397"
                    font.pixelSize: 12
                }
            }

            AbstractButton {
                id: closeButton
                anchors.top: parent.top
                anchors.right: parent.right
                width: 32
                height: 32
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: "Закрыть"
                ToolTip.delay: 600
                onClicked: dialog.cancelDialog()
                contentItem: Canvas {
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()
                    onPaint: {
                        const context = getContext("2d")
                        const centerX = width / 2
                        const centerY = height / 2
                        context.clearRect(0, 0, width, height)
                        context.strokeStyle = "#526273"
                        context.lineWidth = 1.8
                        context.lineCap = "round"
                        context.beginPath()
                        context.moveTo(centerX - 5, centerY - 5)
                        context.lineTo(centerX + 5, centerY + 5)
                        context.moveTo(centerX + 5, centerY - 5)
                        context.lineTo(centerX - 5, centerY + 5)
                        context.stroke()
                    }
                }
                background: Rectangle {
                    radius: 8
                    color: "#e8edf3"
                    opacity: closeButton.hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 100 } }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 8

            TextField {
                id: videoFileField
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                enabled: !dialog.controller.running
                text: dialog.controller.fileName
                placeholderText: "Путь к видеофайлу"
                selectByMouse: true
                leftPadding: 12
                rightPadding: 12
                verticalAlignment: TextInput.AlignVCenter
                onTextEdited: dialog.controller.fileName = text
                background: Rectangle {
                    radius: 8
                    color: videoFileField.enabled ? "#f7f9fc" : "#eef2f6"
                    border.color: videoFileField.activeFocus ? "#65aee8" : "#cbd7e2"
                }
            }

            AbstractButton {
                id: browseButton
                Layout.preferredWidth: 42
                Layout.preferredHeight: 40
                enabled: !dialog.controller.running
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: "Выбрать видеофайл"
                ToolTip.delay: 600
                onClicked: dialog.controller.browseFile()
                contentItem: Text {
                    text: "…"
                    color: browseButton.enabled ? "#315e7b" : "#8a98a7"
                    font.pixelSize: 19
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: browseButton.hovered && browseButton.enabled ? "#d9efff" : "#eaf6ff"
                    border.color: "#afd7f3"
                }
            }

            ActionButton {
                text: "Извлечь кадры"
                Layout.preferredHeight: 40
                enabled: dialog.controller.canStart
                onClicked: {
                    thumbsView.clearSelection()
                    dialog.controller.start()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 10

            Label { text: "Количество кадров:"; color: "#526273" }
            SpinBox {
                id: frameCountBox
                Layout.preferredWidth: 122
                Layout.preferredHeight: 40
                from: 1
                to: 10000
                editable: true
                enabled: !dialog.controller.running
                value: dialog.controller.frameCount
                onValueModified: dialog.controller.frameCount = value
                leftPadding: 10
                rightPadding: 62

                contentItem: TextInput {
                    text: frameCountBox.textFromValue(frameCountBox.value, frameCountBox.locale)
                    color: frameCountBox.enabled ? "#1f2c3d" : "#8794a3"
                    selectionColor: "#9fd5f6"
                    selectedTextColor: "#1f2c3d"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    readOnly: !frameCountBox.editable
                    validator: frameCountBox.validator
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                }
                background: Rectangle {
                    radius: 10
                    color: frameCountBox.hovered ? "#ffffff" : "#f1f6fb"
                    border.color: frameCountBox.activeFocus ? "#65aee8" : "#c6d5e3"
                }
                down.indicator: Rectangle {
                    x: frameCountBox.width - 60
                    y: 1
                    width: 30
                    height: frameCountBox.height - 2
                    color: frameCountBox.down.pressed ? "#cce8fa"
                                                      : frameCountBox.down.hovered ? "#e1f2fd" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "−"
                        color: frameCountBox.enabled ? "#426783" : "#9ba7b4"
                        font.pixelSize: 18
                    }
                }
                up.indicator: Rectangle {
                    x: frameCountBox.width - 30
                    y: 1
                    width: 29
                    height: frameCountBox.height - 2
                    radius: 9
                    color: frameCountBox.up.pressed ? "#cce8fa"
                                                    : frameCountBox.up.hovered ? "#e1f2fd" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "+"
                        color: frameCountBox.enabled ? "#426783" : "#9ba7b4"
                        font.pixelSize: 17
                    }
                }
            }
            Item { Layout.fillWidth: true }
            Label { text: "Движок:"; color: "#526273" }
            ComboBox {
                id: engineBox
                Layout.preferredWidth: 190
                Layout.preferredHeight: 40
                enabled: !dialog.controller.running
                model: dialog.controller.videoEngines
                currentIndex: Math.max(0, dialog.controller.videoEngines.indexOf(dialog.controller.selectedEngine))
                onActivated: dialog.controller.selectedEngine = currentText
                leftPadding: 12
                rightPadding: 42

                contentItem: Text {
                    text: engineBox.displayText
                    color: engineBox.enabled ? "#1f2c3d" : "#8794a3"
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 10
                    color: engineBox.hovered && engineBox.enabled ? "#ffffff" : "#f1f6fb"
                    border.color: engineBox.activeFocus ? "#65aee8" : "#c6d5e3"
                }
                indicator: Item {
                    x: engineBox.width - width - 7
                    anchors.verticalCenter: parent.verticalCenter
                    width: 30
                    height: 30
                    rotation: engineBox.popup.visible ? 180 : 0

                    Behavior on rotation {
                        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                    }

                    Canvas {
                        id: engineChevron
                        readonly property color strokeColor: engineBox.popup.visible ? "#287caf" : "#58708b"
                        anchors.centerIn: parent
                        width: 14
                        height: 9
                        onStrokeColorChanged: requestPaint()
                        onPaint: {
                            const context = getContext("2d")
                            context.clearRect(0, 0, width, height)
                            context.strokeStyle = strokeColor
                            context.lineWidth = 2
                            context.lineCap = "round"
                            context.lineJoin = "round"
                            context.beginPath()
                            context.moveTo(1.5, 2)
                            context.lineTo(width / 2, height - 2)
                            context.lineTo(width - 1.5, 2)
                            context.stroke()
                        }
                    }
                }
                popup: Popup {
                    y: engineBox.height + 5
                    width: engineBox.width
                    implicitHeight: Math.min(contentItem.implicitHeight + 12, 280)
                    padding: 6
                    background: Rectangle {
                        radius: 12
                        color: "#ffffff"
                        border.color: "#c8d7e5"
                    }
                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: engineBox.popup.visible ? engineBox.delegateModel : null
                        ScrollIndicator.vertical: ScrollIndicator {}
                    }
                }
                delegate: ItemDelegate {
                    id: engineDelegate
                    required property string modelData
                    required property int index
                    width: engineBox.width - 12
                    height: 38
                    highlighted: engineBox.highlightedIndex === index
                    contentItem: Text {
                        text: engineDelegate.modelData
                        color: "#24354a"
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle {
                        radius: 8
                        color: engineDelegate.highlighted ? "#e5f3ff" : "transparent"
                    }
                }
            }
        }

        ThumbsView {
            id: thumbsView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            controller: dialog.controller
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            visible: dialog.controller.errorText.length > 0
            text: dialog.controller.errorText
            color: "#c43d4b"
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.bottomMargin: 18
            spacing: 8

            AbstractButton {
                id: stopButton
                visible: dialog.controller.running
                text: "Остановить"
                hoverEnabled: true
                leftPadding: 16
                rightPadding: 16
                topPadding: 9
                bottomPadding: 9
                onClicked: dialog.controller.stop()
                contentItem: Text {
                    text: stopButton.text
                    color: "#a3474f"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: stopButton.hovered ? "#ffe4e7" : "#fff1f2"
                    border.color: "#efc3c8"
                }
            }
            BusyIndicator {
                visible: dialog.controller.running
                running: visible
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
            }
            Item { Layout.fillWidth: true }
            AbstractButton {
                id: cancelButton
                text: "Отмена"
                hoverEnabled: true
                leftPadding: 18
                rightPadding: 18
                topPadding: 9
                bottomPadding: 9
                onClicked: dialog.cancelDialog()
                contentItem: Text {
                    text: cancelButton.text
                    color: "#3f4d5e"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { radius: 8; color: cancelButton.hovered ? "#e8edf3" : "#f1f4f7" }
            }
            ActionButton {
                text: "Добавить кадры"
                Layout.preferredHeight: 38
                enabled: dialog.controller.canAccept
                onClicked: dialog.acceptDialog()
            }
        }
    }
}
