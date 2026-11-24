import QtQuick
import QtQuick.Controls

// Shared GIF actions for the grid context menu and the viewer menu. The owner
// supplies the target URL; delete/rename are forwarded as signals because the
// dialogs live in Main.qml and the viewer closes itself after those actions.
Menu {
    id: gifMenu

    property url gifUrl

    signal deleteRequested()
    signal renameRequested()

    MenuItem {
        text: qsTr("Share")
        // The share sheet only exists on Android. Hiding the item also removes
        // its row from the menu, hence the explicit zero height.
        visible: Qt.platform.os === "android"
        height: visible ? implicitHeight : 0
        onTriggered: ClipboardService.shareFile(gifMenu.gifUrl)
    }
    MenuItem {
        text: qsTr("Copy GIF")
        onTriggered: ClipboardService.copyFile(gifMenu.gifUrl)
    }
    MenuItem {
        text: qsTr("Delete")
        onTriggered: gifMenu.deleteRequested()
    }
    MenuItem {
        text: qsTr("Rename")
        onTriggered: gifMenu.renameRequested()
    }
}
