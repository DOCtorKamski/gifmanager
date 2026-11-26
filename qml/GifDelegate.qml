import QtQuick

Item {
    id: root

    required property string fileName
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
    readonly property bool mutating: GifController.mutatingUrl.toString()
                                     === root.fileUrl.toString()
    // Decoding and animating GIFs is too expensive on Android, so the grid
    // stays on the static first-frame thumbnail there. The full-size viewer
    // keeps animating normally.
    readonly property bool animatedPreviewEnabled: Qt.platform.os !== "android"

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
        color: "#999999"
        border.color: "#444444"
        border.width: 1

        Image {
            anchors.fill: parent
            anchors.margins: 1
            source: (root.thumbnailReady && !root.mutating)
                    ? root.thumbnailUrl
                    : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: true
            // Hide the static thumbnail once the animation is showing frames;
            // otherwise its first frame shows through transparent pixels.
            visible: root.thumbnailReady && !root.mutating
                     && (!root.animatedPreviewEnabled
                         || animation.status !== AnimatedImage.Ready)
        }

        AnimatedImage {
            id: animation
            anchors.fill: parent
            anchors.margins: 1
            // playbackUrl() maps content:// URIs to a cached file:// copy on
            // Android; on desktop it returns the URL unchanged. Android keeps
            // the grid static, so the GIF is only decoded in the viewer.
            source: (root.animatedPreviewEnabled && root.onScreen
                     && !root.mutating)
                    ? GifController.playbackUrl(root.fileUrl)
                    : ""
            // Pause instead of stopping: QMovie::stop() resets playback to the
            // first frame, while setPaused() resumes from the current frame.
            playing: root.animatedPreviewEnabled && root.onScreen
                     && !root.mutating
            paused: root.scrolling
            fillMode: AnimatedImage.PreserveAspectFit
            asynchronous: true
            cache: false
            sourceSize: Qt.size(Math.max(1, width), Math.max(1, height))
            visible: root.animatedPreviewEnabled
                     && status === AnimatedImage.Ready
        }
    }

    MouseArea {
        id: interaction
        anchors.fill: frame
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        // Touch events arrive here as synthesized mouse clicks on Android,
        // so the desktop mouse behavior doubles as touch: tap opens,
        // long-press opens the context menu.
        property bool longPress: false

        onClicked: function(mouse) {
            if (interaction.longPress) {
                interaction.longPress = false
                return
            }
            if (mouse.button === Qt.LeftButton) {
                root.openRequested()
            } else {
                const p = mapToItem(null, mouse.x, mouse.y)
                root.contextMenuRequested(p.x, p.y)
            }
        }

        onPressAndHold: function(mouse) {
            interaction.longPress = true
            const p = mapToItem(null, mouse.x, mouse.y)
            root.contextMenuRequested(p.x, p.y)
        }

        onCanceled: interaction.longPress = false

        onEntered: root.hoverChanged(true)
        onExited: root.hoverChanged(false)
    }
}
