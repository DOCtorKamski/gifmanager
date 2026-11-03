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
        if (AppSettings.hasWindowGeometry) {
            const geometry = AppSettings.windowGeometry
            x = geometry.x
            y = geometry.y
            width = geometry.width
            height = geometry.height
        }
    }

    onClosing: {
        AppSettings.saveWindowGeometry(x, y, width, height)
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
                    onClicked: {
                        const error = GifController.loadDefaultFolder()
                        if (error.length > 0)
                            root.showError(error)
                    }
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
        model: GifController.proxyModel
        cellWidth: Math.max(1, Math.floor((width
                       - (AppSettings.columns - 1) * AppSettings.gridHorizontalSpacing)
                       / Math.max(1, AppSettings.columns)))
        cellHeight: Math.max(1, AppSettings.thumbnailHeight
                            + AppSettings.gridVerticalSpacing)
        cacheBuffer: Math.max(1, cellHeight * 2)
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            blocking: true

            onWheel: function(event) {
                const pixels_per_notch = AppSettings.gridScrollSpeed
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
            interval: Math.max(30, AppSettings.visibilityDebounceMs)
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
            thumbWidth: AppSettings.thumbnailWidth
            thumbHeight: AppSettings.thumbnailHeight

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
            onTriggered: GifController.copyToClipboard(root.contextMenuPath)
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
        visible: grid.count === 0 && !GifController.loading
        text: GifController.currentFolder.length > 0
              ? qsTr("No GIFs found")
              : qsTr("Choose a folder with GIFs")
        color: "#777777"
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                Layout.fillWidth: true
                text: GifController.loading
                      ? qsTr("Loading: %1/%2")
                        .arg(GifController.progressCurrent)
                        .arg(GifController.progressTotal)
                      : qsTr("Ready")
            }
            ProgressBar {
                visible: GifController.loading
                from: 0
                to: Math.max(1, GifController.progressTotal)
                value: GifController.progressCurrent
                Layout.preferredWidth: 220
            }
        }
    }

    Timer {
        id: searchTimer
        interval: AppSettings.searchDebounceMs
        repeat: false
        onTriggered: {
            root.hoveredPath = ""
            GifController.setFilterQuery(searchField.text)
            grid.positionViewAtBeginning()
            viewportTimer.restart()
        }
    }

    Shortcut {
        sequences: ["Ctrl+C"]
        enabled: root.hoveredPath.length > 0 && !searchField.activeFocus
        onActivated: GifController.copyToClipboard(root.hoveredPath)
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
        currentFolder: GifController.currentFolderUrl
        onAccepted: {
            const error = GifController.loadFolderUrl(selectedFolder)
            if (error.length > 0)
                root.showError(error)
        }
    }

    SettingsDialog {
        id: settingsDialog
        onSettingsApplied: GifController.applySettings()
    }

    GifViewerWindow {
        id: viewerWindow
        transientParent: root
        filePath: root.viewerPath
        fileUrl: GifController.localFileUrl(root.viewerPath)
        fileName: GifController.displayName(root.viewerPath)

        onCloseRequested: root.closeViewer()
        onCopyRequested: GifController.copyToClipboard(root.viewerPath)
        onDeleteRequested: root.requestDelete(root.viewerPath)
        onRenameRequested: root.requestRename(root.viewerPath)
    }

    ConfirmDeleteDialog {
        id: deleteDialog
        fileName: GifController.displayName(root.pendingDeletePath)

        onDeleteConfirmed: {
            const error = GifController.deleteFile(root.pendingDeletePath)
            if (error.length > 0)
                root.showError(error)
        }
    }

    RenameGifDialog {
        id: renameDialog
        currentName: GifController.baseNameForPath(root.pendingRenamePath)

        onRenameSubmitted: function(new_name) {
            const error = GifController.renameFile(root.pendingRenamePath, new_name)
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
