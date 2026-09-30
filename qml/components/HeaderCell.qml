import QtQuick
import QtQuick.Controls
import PlainRun
import PlainRun.Core

// Clickable column heading in the run history; shows the sort direction.
AbstractButton {
    id: header

    property int column: 0
    property int alignment: Text.AlignRight
    readonly property bool active: App.sortColumn === column

    focusPolicy: Qt.NoFocus
    padding: 0
    onClicked: App.sortBy(column)
    Accessible.role: Accessible.ColumnHeader
    Accessible.name: text + (active ? (App.sortAscending ? qsTr(", sorted ascending") : qsTr(", sorted descending")) : "")

    ToolTip.visible: hovered
    ToolTip.delay: 700
    ToolTip.text: qsTr("Sort by %1").arg(text.toLowerCase())

    contentItem: SectionLabel {
        text: header.text + (header.active ? (App.sortAscending ? " ↑" : " ↓") : "")
        horizontalAlignment: header.alignment
        color: header.active || header.hovered ? Theme.text : Theme.mutedText
    }
    background: null
}
