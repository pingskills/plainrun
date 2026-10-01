import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun.Core

// Plain text tabs with an accent underline.
RowLayout {
    id: selector

    property int current: 0
    property var tabs: []
    property string name
    signal chosen(int index)

    spacing: 2
    Accessible.role: Accessible.PageTabList
    Accessible.name: name

    Repeater {
        model: selector.tabs

        delegate: AbstractButton {
            id: tab
            required property var modelData
            required property int index
            readonly property bool selected: selector.current === index

            text: modelData.text
            checkable: false
            focusPolicy: Qt.StrongFocus
            padding: 6
            leftPadding: 10
            rightPadding: 10
            onClicked: selector.chosen(index)
            Accessible.role: Accessible.PageTab
            Accessible.name: modelData.tip
            Accessible.checked: selected

            ToolTip.visible: hovered
            ToolTip.delay: 700
            ToolTip.text: modelData.tip

            contentItem: Label {
                text: tab.text
                horizontalAlignment: Text.AlignHCenter
                color: tab.selected ? Theme.text : Theme.mutedText
                font.weight: tab.selected ? Font.DemiBold : Font.Normal
            }
            background: Rectangle {
                color: tab.hovered && !tab.selected ? Theme.surface : "transparent"
                border.width: tab.visualFocus ? 1 : 0
                border.color: Theme.accent
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 2
                    color: Theme.accent
                    visible: tab.selected
                }
            }
        }
    }
}
