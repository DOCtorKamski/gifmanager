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

    property url viewerUrl
    property string viewerFileName: ""
    property url hoveredUrl
    property url pendingDeleteUrl
    property string pendingDeleteFileName: ""
    property url pendingRenameUrl
    property string pendingRenameFileName: ""
    property url contextMenuUrl
    property string contextMenuFileName: ""

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

            onOpenRequested: root.toggleViewer(fileUrl, fileName)
            onContextMenuRequested: function(sceneX, sceneY) {
                root.contextMenuUrl = fileUrl
                root.contextMenuFileName = fileName
                const p = grid.mapFromItem(null, sceneX, sceneY)
                gridContextMenu.popup(grid, p.x, p.y)
            }
            onHoverChanged: function(hovered) {
                if (hovered) {
                    root.hoveredUrl = fileUrl
                } else if (root.hoveredUrl.toString() === fileUrl.toString()) {
                    root.hoveredUrl = ""
                }
            }
        }
    }

    Menu {
        id: gridContextMenu
        MenuItem {
            text: qsTr("Copy GIF")
            onTriggered: ClipboardService.copyFile(root.contextMenuUrl)
        }
        MenuItem {
            text: qsTr("Delete")
            onTriggered: root.requestDelete(root.contextMenuUrl,
                                            root.contextMenuFileName)
        }
        MenuItem {
            text: qsTr("Rename")
            onTriggered: root.requestRename(root.contextMenuUrl,
                                            root.contextMenuFileName)
        }
    }

    Label {
        anchors.centerIn: parent
        visible: grid.count === 0 && !GifController.loading
        text: GifController.hasFolder
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
            root.hoveredUrl = ""
            GifController.setFilterQuery(searchField.text)
            grid.positionViewAtBeginning()
            viewportTimer.restart()
        }
    }

    Shortcut {
        sequences: ["Ctrl+C"]
        enabled: root.hoveredUrl.toString().length > 0 && !searchField.activeFocus
        onActivated: ClipboardService.copyFile(root.hoveredUrl)
    }

    Shortcut {
        sequences: ["Ctrl+F"]
        onActivated: {
            searchField.forceActiveFocus()
            searchField.selectAll()
        }
    }

    function toggleViewer(url, name) {
        if (root.viewerUrl.toString() === url.toString()) {
            closeViewer()
        } else {
            root.viewerUrl = url
            root.viewerFileName = name
            viewerWindow.show()
        }
    }

    function closeViewer() {
        root.viewerUrl = ""
        root.viewerFileName = ""
        viewerWindow.hide()
    }

    function requestDelete(url, name) {
        root.pendingDeleteUrl = url
        root.pendingDeleteFileName = name
        deleteDialog.open()
    }

    function requestRename(url, name) {
        root.pendingRenameUrl = url
        root.pendingRenameFileName = name
        renameDialog.open()
    }

    function showError(message) {
        errorDialog.message = message
        errorDialog.open()
    }

    Connections {
        target: GifController

        function onFilterCleared() {
            searchField.text = ""
            searchTimer.stop()
            root.hoveredUrl = ""
        }

        function onFileAboutToChange(url) {
            if (root.hoveredUrl.toString() === url.toString())
                root.hoveredUrl = ""
            if (root.viewerUrl.toString() === url.toString())
                root.closeViewer()
        }
    }

    FolderDialog {
        id: folderDialog
        title: qsTr("Select GIF Folder")
        currentFolder: Qt.platform.os === "android"
                       ? "" : GifController.currentFolderUrl
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
        fileUrl: root.viewerUrl
        fileName: root.viewerFileName

        onCloseRequested: root.closeViewer()
        onCopyRequested: ClipboardService.copyFile(root.viewerUrl)
        onDeleteRequested: root.requestDelete(root.viewerUrl, root.viewerFileName)
        onRenameRequested: root.requestRename(root.viewerUrl, root.viewerFileName)
    }

    ConfirmDeleteDialog {
        id: deleteDialog
        fileName: root.pendingDeleteFileName

        onDeleteConfirmed: {
            const error = GifController.deleteFile(root.pendingDeleteUrl)
            if (error.length > 0)
                root.showError(error)
        }
    }

    RenameGifDialog {
        id: renameDialog
        currentName: PathUtils.baseName(root.pendingRenameFileName)

        onRenameSubmitted: function(new_name) {
            const error = GifController.renameFile(root.pendingRenameUrl, new_name)
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
