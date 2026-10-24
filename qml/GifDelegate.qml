import QtQuick

Item {
    id: root

    required property string filePath
    required property url fileUrl
    required property url thumbnailUrl
    required property bool thumbnailReady
    property real thumbWidth: 200
    property real thumbHeight: 160
    property real viewTop: 0
    property real viewBottom: 0
    property bool scrolling: false
    property real cellWidth: thumbWidth
    property real cellHeight: thumbHeight

    readonly property bool onScreen: visible
                                      && (y + height > viewTop)
                                      && (y < viewBottom)
    readonly property bool removing: gifController.removingPath === filePath

    signal openRequested()
    signal contextMenuRequested(real sceneX, real sceneY)
    signal hoverChanged(bool hovered)

    width: cellWidth
    height: cellHeight

    Rectangle {
        id: frame
        width: Math.min(root.thumbWidth, root.width)
        height: Math.min(root.thumbHeight, root.height)
        anchors.centerIn: parent
        color: "#999"
        border.color: "#444"
        border.width: 1

        Image {
            anchors.fill: parent
            anchors.margins: 1
            source: (root.thumbnailReady && !root.removing)
                    ? root.thumbnailUrl
                    : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: true
        }

        AnimatedImage {
            anchors.fill: parent
            anchors.margins: 1
            source: (root.onScreen && !root.removing)
                    ? root.fileUrl
                    : ""
            playing: root.onScreen && !root.scrolling && !root.removing
            fillMode: AnimatedImage.PreserveAspectFit
            asynchronous: true
            cache: false
            sourceSize: Qt.size(Math.max(1, width), Math.max(1, height))
            visible: status === AnimatedImage.Ready
        }
    }

    MouseArea {
        anchors.fill: frame
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true

        onClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                root.openRequested()
            } else {
                const p = mapToItem(null, mouse.x, mouse.y)
                root.contextMenuRequested(p.x, p.y)
            }
        }

        onEntered: root.hoverChanged(true)
        onExited: root.hoverChanged(false)
    }
}
