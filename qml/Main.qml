import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    width: 800
    height: 600
    minimumWidth: 480
    minimumHeight: 360
    visible: true
    title: qsTr("Gif Manager")

    property string viewerPath: ""
    property string hoveredPath: ""
    property string pendingDeletePath: ""
    property string pendingRenamePath: ""
    property string contextMenuPath: ""

    Component.onCompleted: {
        if (appSettings.hasWindowGeometry) {
            const geometry = appSettings.windowGeometry
            x = geometry.x
            y = geometry.y
            width = geometry.width
            height = geometry.height
        }
    }

    onClosing: {
        appSettings.saveWindowGeometry(x, y, width, height)
        closeViewer()
    }

    header: ColumnLayout {
        spacing: 0

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                Button {
                    text: qsTr("Choose folder with GIFs")
                    onClicked: folderDialog.open()
                }
                Button {
                    text: qsTr("Settings")
                    onClicked: settingsDialog.open()
                }
                Button {
                    text: qsTr("Go to default folder")
                    onClicked: gifController.loadDefaultFolder()
                }
                Item { Layout.fillWidth: true }
            }
        }

        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search GIFs by name...")
                    onTextChanged: searchTimer.restart()
                }
            }
        }
    }

    GridView {
        id: grid
        anchors.fill: parent
        clip: true
        model: gifController.proxyModel
        cellWidth: Math.max(1, Math.floor((width
                       - (appSettings.columns - 1) * appSettings.gridHorizontalSpacing)
                       / Math.max(1, appSettings.columns)))
        cellHeight: Math.max(1, appSettings.thumbnailHeight
                            + appSettings.gridVerticalSpacing)
        cacheBuffer: Math.max(1, cellHeight * 2)
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            blocking: true

            onWheel: function(event) {
                const pixels_per_notch = appSettings.gridScrollSpeed
                const delta = event.pixelDelta.y !== 0
                      ? event.pixelDelta.y * pixels_per_notch / 72
                      : event.angleDelta.y / 120 * pixels_per_notch
                if (delta === 0)
                    return

                const max_y = Math.max(0, grid.contentHeight - grid.height)
                grid.contentY = Math.max(0, Math.min(max_y, grid.contentY - delta))
            }
        }

        property bool scrolling: false
        property real viewTop: 0
        property real viewBottom: 0

        function refreshViewport() {
            viewTop = contentY
            viewBottom = contentY + height
        }

        Timer {
            id: viewportTimer
            interval: Math.max(30, appSettings.visibilityDebounceMs)
            repeat: false
            onTriggered: {
                grid.refreshViewport()
                grid.scrolling = false
            }
        }

        onContentYChanged: {
            scrolling = true
            viewportTimer.restart()
        }
        onHeightChanged: viewportTimer.restart()
        Component.onCompleted: {
            refreshViewport()
            viewportTimer.restart()
        }

        delegate: GifDelegate {
            viewTop: grid.viewTop
            viewBottom: grid.viewBottom
            scrolling: grid.scrolling
            cellWidth: grid.cellWidth
            cellHeight: grid.cellHeight
            thumbWidth: appSettings.thumbnailWidth
            thumbHeight: appSettings.thumbnailHeight

            onOpenRequested: root.toggleViewer(filePath)
            onContextMenuRequested: function(sceneX, sceneY) {
                root.contextMenuPath = filePath
                const p = grid.mapFromItem(null, sceneX, sceneY)
                gridContextMenu.popup(grid, p.x, p.y)
            }
            onHoverChanged: function(hovered) {
                if (hovered) {
                    root.hoveredPath = filePath
                } else if (root.hoveredPath === filePath) {
                    root.hoveredPath = ""
                }
            }
        }
    }

    Menu {
        id: gridContextMenu
        MenuItem {
            text: qsTr("Copy GIF")
            onTriggered: gifController.copyToClipboard(root.contextMenuPath)
        }
        MenuItem {
            text: qsTr("Delete")
            onTriggered: root.requestDelete(root.contextMenuPath)
        }
        MenuItem {
            text: qsTr("Rename")
            onTriggered: root.requestRename(root.contextMenuPath)
        }
    }

    Label {
        anchors.centerIn: parent
        visible: grid.count === 0 && !gifController.loading
        text: gifController.currentFolder.length > 0
              ? qsTr("No GIFs found")
              : qsTr("Choose a folder with GIFs")
        color: "#777"
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                text: gifController.loading
                      ? qsTr("Loading: %1/%2")
                        .arg(gifController.progressCurrent)
                        .arg(gifController.progressTotal)
                      : qsTr("Ready")
            }
            ProgressBar {
                visible: gifController.loading
                from: 0
                to: Math.max(1, gifController.progressTotal)
                value: gifController.progressCurrent
                Layout.preferredWidth: 220
            }
        }
    }

    Timer {
        id: searchTimer
        interval: appSettings.searchDebounceMs
        repeat: false
        onTriggered: {
            root.hoveredPath = ""
            gifController.setFilterQuery(searchField.text)
            grid.positionViewAtBeginning()
            viewportTimer.restart()
        }
    }

    Shortcut {
        sequences: ["Ctrl+C"]
        enabled: root.hoveredPath.length > 0 && !searchField.activeFocus
        onActivated: gifController.copyToClipboard(root.hoveredPath)
    }

    Shortcut {
        sequences: ["Ctrl+F"]
        onActivated: {
            searchField.forceActiveFocus()
            searchField.selectAll()
        }
    }

    function toggleViewer(path) {
        if (viewerPath === path) {
            closeViewer()
        } else {
            viewerPath = path
            viewerWindow.show()
        }
    }

    function closeViewer() {
        viewerPath = ""
        viewerWindow.hide()
    }

    function requestDelete(path) {
        pendingDeletePath = path
        deleteDialog.open()
    }

    function requestRename(path) {
        pendingRenamePath = path
        renameDialog.open()
    }

    function showError(message) {
        errorDialog.message = message
        errorDialog.open()
    }

    Connections {
        target: gifController

        function onFilterCleared() {
            searchField.text = ""
            searchTimer.stop()
            root.hoveredPath = ""
        }

        function onOperationError(message) {
            root.showError(message)
        }

        function onFileAboutToBeRemoved(path) {
            if (root.hoveredPath === path)
                root.hoveredPath = ""
            if (root.viewerPath === path)
                root.closeViewer()
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Select GIF Folder")
        currentFolder: gifController.currentFolderUrl
        onAccepted: gifController.loadFolderUrl(selectedFolder)
    }

    SettingsDialog {
        id: settingsDialog
        onSettingsApplied: gifController.applySettings()
    }

    GifViewerWindow {
        id: viewerWindow
        transientParent: root
        filePath: root.viewerPath
        fileUrl: gifController.localFileUrl(root.viewerPath)
        fileName: gifController.displayName(root.viewerPath)

        onCloseRequested: root.closeViewer()
        onCopyRequested: gifController.copyToClipboard(root.viewerPath)
        onDeleteRequested: root.requestDelete(root.viewerPath)
        onRenameRequested: root.requestRename(root.viewerPath)
    }

    ConfirmDeleteDialog {
        id: deleteDialog
        fileName: gifController.displayName(root.pendingDeletePath)

        onDeleteConfirmed: {
            const error = gifController.deleteFile(root.pendingDeletePath)
            if (error.length > 0)
                root.showError(error)
        }
    }

    RenameGifDialog {
        id: renameDialog
        currentName: gifController.baseNameForPath(root.pendingRenamePath)

        onRenameSubmitted: function(new_name) {
            const error = gifController.renameFile(root.pendingRenamePath, new_name)
            if (error.length > 0)
                renameDialog.errorText = error
            else
                renameDialog.close()
        }
    }

    ErrorDialog {
        id: errorDialog
    }
}
