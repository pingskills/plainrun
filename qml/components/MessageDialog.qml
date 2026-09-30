import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun.Core

// Modal message with optional details (e.g. the rows a CSV import rejected).
Dialog {
    id: dialog

    property string text
    property string details

    function show(title, text, details) {
        dialog.title = title
        dialog.text = text
        dialog.details = details || ""
        open()
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(520, parent.width - 48)
    modal: true
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, Theme.dark ? 0.45 : 0.25) }
    closePolicy: Popup.CloseOnEscape
    padding: 20

    onOpened: okButton.forceActiveFocus()

    contentItem: ColumnLayout {
        spacing: 10
        Label {
            Layout.fillWidth: true
            text: dialog.text
            wrapMode: Text.Wrap
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(detailsArea.implicitHeight, 200)
            visible: dialog.details.length > 0
            TextArea {
                id: detailsArea
                text: dialog.details
                readOnly: true
                wrapMode: Text.Wrap
                font.family: "monospace"
                font.pointSize: Theme.fontSize * 0.9
                Accessible.name: qsTr("Details")
            }
        }
    }

    footer: RowLayout {
        Item { Layout.fillWidth: true }
        Button {
            id: okButton
            text: qsTr("OK")
            Layout.rightMargin: 16
            Layout.bottomMargin: 16
            onClicked: dialog.close()
            Keys.onReturnPressed: clicked()
            Keys.onEnterPressed: clicked()
        }
    }
}
