import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property string currentName: ""
    property string errorText: ""

    signal renameSubmitted(string new_name)

    title: qsTr("Rename GIF")
    modal: true
    width: 420
    anchors.centerIn: parent

    onOpened: {
        renameField.text = root.currentName
        renameField.selectAll()
        renameField.forceActiveFocus()
        root.errorText = ""
    }

    function submit() {
        root.renameSubmitted(renameField.text)
    }

    contentItem: ColumnLayout {
        spacing: 8
        Label {
            text: qsTr("Enter new name (without extension):")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        TextField {
            id: renameField
            Layout.fillWidth: true
            onAccepted: root.submit()
        }
        Label {
            text: root.errorText
            color: "#b00020"
            visible: text.length > 0
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    footer: RowLayout {
        spacing: 8
        Item { Layout.fillWidth: true }
        Button {
            text: qsTr("Cancel")
            onClicked: root.close()
        }
        Button {
            text: qsTr("OK")
            onClicked: root.submit()
        }
    }
}
