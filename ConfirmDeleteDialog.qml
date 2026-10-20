import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property string fileName: ""

    signal deleteConfirmed()

    title: qsTr("Confirm Deletion")
    modal: true
    width: 420
    anchors.centerIn: parent

    contentItem: Label {
        text: qsTr("Are you sure you want to delete '%1'?").arg(root.fileName)
        wrapMode: Text.WordWrap
        width: root.width - 48
    }

    footer: RowLayout {
        spacing: 8
        Item { Layout.fillWidth: true }
        Button {
            text: qsTr("No")
            onClicked: root.close()
        }
        Button {
            text: qsTr("Yes")
            onClicked: {
                root.close()
                root.deleteConfirmed()
            }
        }
    }
}
