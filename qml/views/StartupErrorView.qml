import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

// Shown instead of the main view when the database cannot be opened.
// PlainRun never deletes or recreates data to "recover".
Pane {
    background: Rectangle { color: Theme.background }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 560)
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTr("PlainRun could not open your running log")
            font.pointSize: Theme.fontSize * 1.4
            font.weight: Font.DemiBold
            wrapMode: Text.Wrap
        }
        TextEdit {
            Layout.fillWidth: true
            text: App.startupError
            readOnly: true
            selectByMouse: true
            wrapMode: Text.Wrap
            color: Theme.text
            font: Qt.application.font
            Accessible.name: qsTr("Error details")
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Nothing has been changed or deleted. Your data folder is:")
            color: Theme.mutedText
            wrapMode: Text.Wrap
        }
        TextEdit {
            Layout.fillWidth: true
            text: App.dataDirectory
            readOnly: true
            selectByMouse: true
            wrapMode: Text.WrapAnywhere
            color: Theme.text
            font: Qt.application.font
        }
        RowLayout {
            Layout.topMargin: 8
            spacing: 8
            Button {
                text: qsTr("Open Data Folder")
                onClicked: Qt.openUrlExternally("file://" + App.dataDirectory)
            }
            Button {
                text: qsTr("Quit")
                onClicked: Qt.quit()
            }
        }
    }
}
