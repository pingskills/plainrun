import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

// One figure in the summary strip: heading, a large value and two quiet lines.
ColumnLayout {
    id: cell

    property alias label: heading.text
    property string value
    property string detail
    property string secondary
    property real valueScale: 1.55

    spacing: 2
    Accessible.role: Accessible.StaticText
    Accessible.name: [label, value, detail, secondary].filter(s => s.length > 0).join(", ")

    SectionLabel {
        id: heading
        Layout.fillWidth: true
        elide: Text.ElideRight
    }
    Label {
        Layout.fillWidth: true
        text: cell.value
        font.pointSize: Theme.fontSize * cell.valueScale
        font.weight: Font.Medium
        font.features: { "tnum": 1 }
        elide: Text.ElideRight
    }
    Label {
        Layout.fillWidth: true
        visible: text.length > 0
        text: cell.detail
        font.features: { "tnum": 1 }
        elide: Text.ElideRight
    }
    Label {
        Layout.fillWidth: true
        visible: text.length > 0
        text: cell.secondary
        color: Theme.mutedText
        font.features: { "tnum": 1 }
        elide: Text.ElideRight
    }
}
