import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root

    property url fileUrl
    property string fileName: ""

    signal closeRequested()
    signal copyRequested()
    signal deleteRequested()
    signal renameRequested()

    width: Math.max(400, AppSettings.viewerWidth)
    height: Math.max(400, AppSettings.viewerHeight)
    title: fileName.length > 0 ? fileName : qsTr("GIF Viewer")
    color: "#000000"

    onClosing: root.closeRequested()

    onVisibleChanged: {
        if (visible) {
            width = Math.max(400, AppSettings.viewerWidth)
            height = Math.max(400, AppSettings.viewerHeight)
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Label {
                text: root.fileName
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideMiddle
                color: "#ffffff"
                padding: 4
                background: Rectangle { color: "#333333" }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                AnimatedImage {
                    id: viewerImage
                    anchors.fill: parent
                    // playbackUrl() maps content:// URIs to a cached file://
                    // copy on Android; on desktop it returns the URL unchanged.
                    source: GifController.playbackUrl(root.fileUrl)
                    playing: true
                    paused: !root.visible
                    fillMode: AnimatedImage.PreserveAspectFit
                    asynchronous: true
                    cache: true
                    visible: status === AnimatedImage.Ready
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Failed to load GIF")
                    color: "#dddddd"
                    visible: root.fileUrl.toString().length > 0
                             && viewerImage.status === AnimatedImage.Error
                }
            }
        }

        MouseArea {
            id: closeArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            // Touch events arrive here as synthesized mouse clicks on
            // Android: tap closes the viewer, long-press opens the menu.
            property bool longPress: false

            onClicked: function(mouse) {
                if (closeArea.longPress) {
                    closeArea.longPress = false
                    return
                }
                if (mouse.button === Qt.LeftButton)
                    root.closeRequested()
                else
                    viewerMenu.popup()
            }

            onPressAndHold: function(mouse) {
                closeArea.longPress = true
                viewerMenu.popup(mouse.x, mouse.y)
            }

            onCanceled: closeArea.longPress = false
        }

        // Android back button closes the viewer before it can pause the app.
        focus: true
        Keys.onBackPressed: root.closeRequested()

        Menu {
            id: viewerMenu
            MenuItem {
                text: qsTr("Share")
                visible: Qt.platform.os === "android"
                height: visible ? implicitHeight : 0
                onTriggered: ClipboardService.shareFile(root.fileUrl)
            }
            MenuItem {
                text: qsTr("Copy GIF")
                onTriggered: root.copyRequested()
            }
            MenuItem {
                text: qsTr("Delete")
                onTriggered: {
                    root.deleteRequested()
                    root.closeRequested()
                }
            }
            MenuItem {
                text: qsTr("Rename")
                onTriggered: {
                    root.renameRequested()
                    root.closeRequested()
                }
            }
        }
    }
}
