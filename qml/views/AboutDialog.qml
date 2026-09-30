import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

Dialog {
    id: about

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(Math.round(Theme.fontSize * 44), parent.width - 48)
    modal: true
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, Theme.dark ? 0.45 : 0.25) }
    closePolicy: Popup.CloseOnEscape
    padding: 24
    title: qsTr("About PlainRun")

    onOpened: closeButton.forceActiveFocus()

    contentItem: ColumnLayout {
        spacing: 8

        RowLayout {
            spacing: 14
            Image {
                source: "qrc:/qt/qml/PlainRun/resources/icons/png/256/io.github.pingskills.plainrun.png"
                sourceSize.width: 48
                sourceSize.height: 48
                Accessible.ignored: true
            }
            ColumnLayout {
                spacing: 0
                Label {
                    text: "PlainRun"
                    font.pointSize: Theme.fontSize * 1.5
                    font.weight: Font.DemiBold
                }
                Label {
                    text: qsTr("Version %1").arg(App.version)
                    color: Theme.mutedText
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 6
            text: qsTr("A simple native running log for Linux.")
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: qsTr("Part of Plain Apps: small, fast, native applications that do one thing well. "
                       + "Your data stays on this computer.")
            color: Theme.mutedText
            wrapMode: Text.Wrap
        }

        SectionLabel { text: qsTr("Data"); Layout.topMargin: 10 }
        TextEdit {
            Layout.fillWidth: true
            text: App.databasePath
            readOnly: true
            selectByMouse: true
            wrapMode: Text.WrapAnywhere
            color: Theme.text
            font: Qt.application.font
            Accessible.name: qsTr("Database location")
        }

        SectionLabel { text: qsTr("Appearance"); Layout.topMargin: 10 }
        Label { text: Theme.sourceDescription }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 10
            text: qsTr("MIT License · github.com/pingskills/plainrun")
            color: Theme.mutedText
            wrapMode: Text.Wrap
        }
    }

    footer: RowLayout {
        spacing: 8
        Button {
            text: qsTr("Open Data Folder")
            Layout.leftMargin: 24
            Layout.bottomMargin: 20
            onClicked: Qt.openUrlExternally("file://" + App.dataDirectory)
        }
        Item { Layout.fillWidth: true }
        Button {
            id: closeButton
            text: qsTr("Close")
            Layout.rightMargin: 24
            Layout.bottomMargin: 20
            onClicked: about.close()
            Keys.onReturnPressed: clicked()
            Keys.onEnterPressed: clicked()
        }
    }
}
