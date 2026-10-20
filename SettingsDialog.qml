import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: root

    signal settingsApplied()

    title: qsTr("Settings")
    modal: true
    width: 480
    height: 560
    anchors.centerIn: parent

    onOpened: loadFromSettings()

    function loadFromSettings() {
        widthSpin.value = appSettings.thumbnailWidth
        heightSpin.value = appSettings.thumbnailHeight
        columnsSpin.value = appSettings.columns
        scrollSpeedSpin.value = appSettings.gridScrollSpeed
        defaultFolderField.text = appSettings.defaultFolder

        viewerWidthSpin.value = appSettings.viewerWidth
        viewerHeightSpin.value = appSettings.viewerHeight

        searchDebounceSpin.value = appSettings.searchDebounceMs
        visibilityDebounceSpin.value = appSettings.visibilityDebounceMs
        gridHSpacingSpin.value = appSettings.gridHorizontalSpacing
        gridVSpacingSpin.value = appSettings.gridVerticalSpacing
    }

    function loadDefaults() {
        const defaults = appSettings.defaultValues()
        widthSpin.value = defaults.thumbnailWidth
        heightSpin.value = defaults.thumbnailHeight
        columnsSpin.value = defaults.columns
        scrollSpeedSpin.value = defaults.gridScrollSpeed
        defaultFolderField.text = defaults.defaultFolder
        searchDebounceSpin.value = defaults.searchDebounceMs
        visibilityDebounceSpin.value = defaults.visibilityDebounceMs
        viewerWidthSpin.value = defaults.viewerWidth
        viewerHeightSpin.value = defaults.viewerHeight
        gridHSpacingSpin.value = defaults.gridHorizontalSpacing
        gridVSpacingSpin.value = defaults.gridVerticalSpacing
    }

    function applyValues() {
        appSettings.thumbnailWidth = widthSpin.value
        appSettings.thumbnailHeight = heightSpin.value
        appSettings.columns = columnsSpin.value
        appSettings.gridScrollSpeed = scrollSpeedSpin.value
        appSettings.defaultFolder = defaultFolderField.text.trim()
        appSettings.searchDebounceMs = searchDebounceSpin.value
        appSettings.visibilityDebounceMs = visibilityDebounceSpin.value
        appSettings.viewerWidth = viewerWidthSpin.value
        appSettings.viewerHeight = viewerHeightSpin.value
        appSettings.gridHorizontalSpacing = gridHSpacingSpin.value
        appSettings.gridVerticalSpacing = gridVSpacingSpin.value
        root.settingsApplied()
    }

    contentItem: ColumnLayout {
        spacing: 12

        TabBar {
            id: tabs
            Layout.fillWidth: true
            TabButton { text: qsTr("Base") }
            TabButton { text: qsTr("Extended") }
        }

        StackLayout {
            currentIndex: tabs.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 10

                Label { text: qsTr("Thumbnail width:") }
                SpinBox {
                    id: widthSpin
                    from: 32
                    to: 512
                    editable: true
                }

                Label { text: qsTr("Thumbnail height:") }
                SpinBox {
                    id: heightSpin
                    from: 32
                    to: 512
                    editable: true
                }

                Label { text: qsTr("Columns:") }
                SpinBox {
                    id: columnsSpin
                    from: 1
                    to: 8
                    editable: true
                }

                Label { text: qsTr("Scroll speed (px per wheel notch):") }
                SpinBox {
                    id: scrollSpeedSpin
                    from: 24
                    to: 264
                    stepSize: 12
                    editable: true
                }

                Label {
                    text: qsTr("Default folder:")
                    Layout.alignment: Qt.AlignTop
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: defaultFolderField
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Browse...")
                        onClicked: browseDialog.open()
                    }
                }

                Item { Layout.fillHeight: true }
            }

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 10

                Label { text: qsTr("Search debounce (ms):") }
                SpinBox {
                    id: searchDebounceSpin
                    from: 50
                    to: 1000
                    editable: true
                }

                Label { text: qsTr("Visibility debounce (ms):") }
                SpinBox {
                    id: visibilityDebounceSpin
                    from: 20
                    to: 500
                    editable: true
                }

                Label { text: qsTr("Viewer width:") }
                SpinBox {
                    id: viewerWidthSpin
                    from: 400
                    to: 1920
                    editable: true
                }

                Label { text: qsTr("Viewer height:") }
                SpinBox {
                    id: viewerHeightSpin
                    from: 400
                    to: 1920
                    editable: true
                }

                Label { text: qsTr("Grid horizontal spacing:") }
                SpinBox {
                    id: gridHSpacingSpin
                    from: 0
                    to: 20
                    editable: true
                }

                Label { text: qsTr("Grid vertical spacing:") }
                SpinBox {
                    id: gridVSpacingSpin
                    from: 0
                    to: 20
                    editable: true
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    footer: RowLayout {
        spacing: 8

        Button {
            text: qsTr("Reset to Defaults")
            onClicked: root.loadDefaults()
        }

        Item { Layout.fillWidth: true }

        Button {
            text: qsTr("Cancel")
            onClicked: root.close()
        }

        Button {
            text: qsTr("Apply")
            onClicked: root.applyValues()
        }

        Button {
            text: qsTr("OK")
            onClicked: {
                root.applyValues()
                root.close()
            }
        }
    }

    FolderDialog {
        id: browseDialog
        title: qsTr("Select Default Folder")
        currentFolder: gifController.localFileUrl(defaultFolderField.text)
        onAccepted: defaultFolderField.text = gifController.localPathFromUrl(selectedFolder)
    }
}
