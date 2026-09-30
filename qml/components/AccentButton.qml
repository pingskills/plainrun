import QtQuick
import QtQuick.Controls
import PlainRun.Core

// The primary action in a view (Add Run, Save Run): a solid accent button.
Button {
    id: control

    padding: 8
    leftPadding: 16
    rightPadding: 16

    contentItem: Label {
        text: control.text
        color: Theme.accentText
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        implicitHeight: 30
        radius: 3
        color: control.down ? Qt.darker(Theme.accent, 1.15) : control.hovered ? Qt.lighter(Theme.accent, 1.08) : Theme.accent
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.text
    }
}
