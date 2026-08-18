pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    required property var model
    required property int itemCount
    property bool busy: false
    property string emptyTitle: qsTr("Items will appear here")
    property string emptyDescription: ""
    property var selectedFrames: ({})
    property int selectionAnchor: -1
    readonly property int selectedFrameCount: Object.keys(selectedFrames).length

    signal removeRequested(var indices)
    signal openRequested(int index)

    function isFrameSelected(index) {
        return selectedFrames[index] === true
    }

    function clearSelection() {
        selectedFrames = ({})
        selectionAnchor = -1
    }

    function revealItem(index, selectItem) {
        if (index < 0 || index >= root.itemCount)
            return
        if (selectItem) {
            const selection = ({})
            selection[index] = true
            selectedFrames = selection
            selectionAnchor = index
        }
        framesView.positionViewAtIndex(index, GridView.Contain)
    }

    function navigateSelection(step) {
        if (root.itemCount === 0)
            return

        let currentIndex = root.selectionAnchor
        if (currentIndex < 0 || currentIndex >= root.itemCount) {
            const selectedIndices = Object.keys(root.selectedFrames)
            currentIndex = selectedIndices.length > 0 ? Number(selectedIndices[0]) : 0
        }

        const nextIndex = Math.max(0, Math.min(root.itemCount - 1, currentIndex + step))
        root.selectFrame(nextIndex, Qt.NoModifier)
        framesView.positionViewAtIndex(nextIndex, GridView.Contain)
    }

    function selectFrame(index, modifiers) {
        framesView.forceActiveFocus()
        const controlPressed = (modifiers & Qt.ControlModifier) !== 0
        const shiftPressed = (modifiers & Qt.ShiftModifier) !== 0
        const nextSelection = ({})

        if (controlPressed) {
            for (const selectedIndex in selectedFrames)
                nextSelection[selectedIndex] = selectedFrames[selectedIndex]
            if (nextSelection[index])
                delete nextSelection[index]
            else
                nextSelection[index] = true
            selectionAnchor = index
        } else if (shiftPressed && selectionAnchor >= 0) {
            const first = Math.min(selectionAnchor, index)
            const last = Math.max(selectionAnchor, index)
            for (let row = first; row <= last; ++row)
                nextSelection[row] = true
        } else {
            nextSelection[index] = true
            selectionAnchor = index
        }
        selectedFrames = nextSelection
    }

    function deleteSelectedFrames() {
        const indices = []
        for (const index in selectedFrames)
            indices.push(Number(index))
        if (indices.length > 0)
            root.removeRequested(indices)
        clearSelection()
    }

    radius: 10
    color: "#f5f8fb"
    border.color: "#d8e2eb"
    clip: true

    Shortcut {
        sequence: "Delete"
        context: Qt.WindowShortcut
        enabled: root.visible && framesView.activeFocus && root.selectedFrameCount > 0
        onActivated: root.deleteSelectedFrames()
    }

    GridView {
        id: framesView
        property point rubberViewportStart: Qt.point(0, 0)
        property point rubberStart: Qt.point(0, 0)

        function clampViewportPoint(point) {
            return Qt.point(Math.max(0, Math.min(width, point.x)),
                            Math.max(0, Math.min(height, point.y)))
        }

        function mapViewportPointToContent(point) {
            const viewportPoint = clampViewportPoint(point)
            return contentItem.mapFromItem(framesView, viewportPoint.x, viewportPoint.y)
        }

        function beginRubberSelection(point) {
            forceActiveFocus()
            rubberViewportStart = clampViewportPoint(point)
            rubberStart = mapViewportPointToContent(point)
            rubberBand.x = framesView.x + rubberViewportStart.x
            rubberBand.y = framesView.y + rubberViewportStart.y
            rubberBand.width = 0
            rubberBand.height = 0
            rubberBand.visible = true
            updateRubberSelection(point)
        }

        function updateRubberSelection(point) {
            if (!rubberBand.visible)
                return

            const viewportPoint = clampViewportPoint(point)
            const contentPoint = mapViewportPointToContent(viewportPoint)
            const left = Math.min(rubberStart.x, contentPoint.x)
            const right = Math.max(rubberStart.x, contentPoint.x)
            const top = Math.min(rubberStart.y, contentPoint.y)
            const bottom = Math.max(rubberStart.y, contentPoint.y)

            rubberBand.x = framesView.x + Math.min(rubberViewportStart.x, viewportPoint.x)
            rubberBand.y = framesView.y + Math.min(rubberViewportStart.y, viewportPoint.y)
            rubberBand.width = Math.max(1, Math.abs(viewportPoint.x - rubberViewportStart.x))
            rubberBand.height = Math.max(1, Math.abs(viewportPoint.y - rubberViewportStart.y))

            const selection = ({})
            const columns = Math.max(1, Math.floor(width / cellWidth))
            for (let index = 0; index < root.itemCount; ++index) {
                const itemLeft = (index % columns) * cellWidth
                const itemTop = Math.floor(index / columns) * cellHeight
                const itemRight = itemLeft + cellWidth - 16
                const itemBottom = itemTop + 118
                if (right >= itemLeft && left <= itemRight && bottom >= itemTop && top <= itemBottom)
                    selection[index] = true
            }
            root.selectedFrames = selection
        }

        function finishRubberSelection() {
            rubberBand.visible = false
            root.selectionAnchor = -1
        }

        anchors.fill: parent
        anchors.margins: 8
        clip: true
        activeFocusOnTab: true
        model: root.model
        cellWidth: Math.max(142, width / Math.max(1, Math.floor(width / 150)))
        cellHeight: 126
        ScrollBar.vertical: ScrollBar {
            width: 8
            policy: ScrollBar.AsNeeded
        }

        Keys.onPressed: function(event) {
            const columns = Math.max(1, Math.floor(framesView.width / framesView.cellWidth))
            switch (event.key) {
            case Qt.Key_Left:
                root.navigateSelection(-1)
                event.accepted = true
                break
            case Qt.Key_Right:
                root.navigateSelection(1)
                event.accepted = true
                break
            case Qt.Key_Up:
                root.navigateSelection(-columns)
                event.accepted = true
                break
            case Qt.Key_Down:
                root.navigateSelection(columns)
                event.accepted = true
                break
            }
        }

        DragHandler {
            target: null
            acceptedButtons: Qt.LeftButton
            property point trackedPosition: centroid.position

            onActiveChanged: {
                if (active)
                    framesView.beginRubberSelection(centroid.pressPosition)
                else
                    framesView.finishRubberSelection()
            }
            onTrackedPositionChanged: {
                if (active)
                    framesView.updateRubberSelection(trackedPosition)
            }
        }

        delegate: Rectangle {
            id: frameDelegate
            required property int index
            required property string time
            required property url source
            readonly property bool selected: root.isFrameSelected(index)
            width: framesView.cellWidth - 16
            height: 118
            radius: 8
            color: selected ? "#dff1fd" : frameMouse.containsMouse ? "#e8f4fb" : "#ffffff"
            border.color: selected ? "#399bd8" : frameMouse.containsMouse ? "#9fcce8" : "#d6e1ea"
            border.width: selected ? 2 : 1

            Image {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 5
                height: 86
                source: frameDelegate.source
                sourceSize.width: width
                sourceSize.height: height
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
            }
            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 5
                text: frameDelegate.time
                color: "#40566b"
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideMiddle
                maximumLineCount: 1
            }
            MouseArea {
                id: frameMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                onClicked: function(mouse) {
                    if (mouse.button === Qt.MiddleButton) {
                        root.removeRequested([frameDelegate.index])
                        root.clearSelection()
                    } else {
                        root.selectFrame(frameDelegate.index, mouse.modifiers)
                    }
                }
                onDoubleClicked: root.openRequested(frameDelegate.index)
            }
        }
    }

    Rectangle {
        id: rubberBand
        visible: false
        z: 20
        color: "#25399bd8"
        border.color: "#399bd8"
        border.width: 1
    }

    Column {
        anchors.centerIn: parent
        spacing: 8
        visible: root.itemCount === 0
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.busy ? qsTr("Extracting frames…") : root.emptyTitle
            color: "#8190a2"
            font.pixelSize: 14
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: !root.busy && root.emptyDescription.length > 0
            text: root.emptyDescription
            color: "#9aa6b4"
            font.pixelSize: 12
        }
    }
}
