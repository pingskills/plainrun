import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun.Core

// One run in the history list: Date | Distance | Time | Pace.
ItemDelegate {
    id: row

    required property int index
    required property var runId
    required property string dateText
    required property string distanceText
    required property string durationText
    required property string paceText
    required property string note

    property bool selected: false
    property int numberColumnWidth: 90

    signal activated()
    signal edit()

    focusPolicy: Qt.NoFocus
    topPadding: 7
    bottomPadding: 7
    leftPadding: 16
    rightPadding: 16
    highlighted: selected

    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("%1, %2, %3, pace %4").arg(dateText).arg(distanceText).arg(durationText).arg(paceText)
                     + (note.length > 0 ? ", " + note : "")
    Accessible.selected: selected

    onClicked: row.activated()
    onDoubleClicked: row.edit()

    background: Rectangle {
        color: row.selected ? Theme.selection : (row.hovered ? Theme.surface : "transparent")
    }

    contentItem: RowLayout {
        spacing: 12
        Label {
            Layout.fillWidth: true
            text: row.dateText
            color: row.selected ? Theme.selectionText : Theme.text
            elide: Text.ElideRight
            font.features: { "tnum": 1 }
        }
        Label {
            Layout.preferredWidth: row.numberColumnWidth
            horizontalAlignment: Text.AlignRight
            text: row.distanceText
            color: row.selected ? Theme.selectionText : Theme.text
            font.features: { "tnum": 1 }
        }
        Label {
            Layout.preferredWidth: row.numberColumnWidth
            horizontalAlignment: Text.AlignRight
            text: row.durationText
            color: row.selected ? Theme.selectionText : Theme.text
            font.features: { "tnum": 1 }
        }
        Label {
            Layout.preferredWidth: row.numberColumnWidth
            horizontalAlignment: Text.AlignRight
            text: row.paceText
            color: row.selected ? Theme.selectionText : Theme.mutedText
            font.features: { "tnum": 1 }
        }
    }
}
