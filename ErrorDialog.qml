import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property string message: ""

    title: qsTr("Error")
    modal: true
    width: 420
    anchors.centerIn: parent

    contentItem: Label {
        text: root.message
        wrapMode: Text.WordWrap
        width: root.width - 48
    }

    footer: RowLayout {
        spacing: 8
        Item { Layout.fillWidth: true }
        Button {
            text: qsTr("OK")
            onClicked: root.close()
        }
    }
}
