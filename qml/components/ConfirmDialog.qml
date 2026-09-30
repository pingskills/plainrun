import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun.Core

// Modal yes/no question. Cancel has focus by default so Enter is safe.
Dialog {
    id: dialog

    property string text
    property string confirmText: qsTr("OK")
    property bool destructive: false
    signal confirmed()

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(460, parent.width - 48)
    modal: true
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, Theme.dark ? 0.45 : 0.25) }
    closePolicy: Popup.CloseOnEscape
    padding: 20

    onOpened: cancelButton.forceActiveFocus()

    contentItem: Label {
        text: dialog.text
        wrapMode: Text.Wrap
    }

    footer: RowLayout {
        spacing: 8
        Item { Layout.fillWidth: true }
        Button {
            id: cancelButton
            text: qsTr("Cancel")
            Layout.bottomMargin: 16
            onClicked: dialog.reject()
            Keys.onReturnPressed: clicked()
            Keys.onEnterPressed: clicked()
        }
        Button {
            id: confirmButton
            text: dialog.confirmText
            Layout.rightMargin: 16
            Layout.bottomMargin: 16
            highlighted: !dialog.destructive
            palette.buttonText: dialog.destructive ? Theme.danger : Theme.accentText
            onClicked: {
                dialog.close()
                dialog.confirmed()
            }
            Keys.onReturnPressed: clicked()
            Keys.onEnterPressed: clicked()
        }
    }
}
