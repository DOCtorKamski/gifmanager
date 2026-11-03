import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root

    property string filePath: ""
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
                    source: root.fileUrl
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
                    visible: root.filePath.length > 0
                             && viewerImage.status === AnimatedImage.Error
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onClicked: function(mouse) {
                if (mouse.button === Qt.LeftButton)
                    root.closeRequested()
                else
                    viewerMenu.popup()
            }
        }

        Menu {
            id: viewerMenu
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
