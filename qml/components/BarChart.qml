import QtQuick
import QtQuick.Controls
import PlainRun
import PlainRun.Core

// A plain bar chart drawn with Qt Quick items (no charting dependency).
// points: [{ label, value, valueText, detail, current }]
Item {
    id: chart

    property string title
    property var points: []
    property int labelEvery: 3
    property string unit: "km"

    readonly property real maxValue: {
        let m = 0
        for (const p of points)
            m = Math.max(m, p.value)
        return m
    }
    readonly property int count: points.length
    readonly property real gap: Math.max(2, Math.round(plot.width / Math.max(1, count) * 0.25))

    implicitHeight: 150
    Accessible.role: Accessible.Chart
    Accessible.name: title + ": " + points.map(p => p.label + " " + p.valueText).join(", ")

    Label {
        id: scaleLabel
        anchors.top: parent.top
        anchors.right: parent.right
        text: chart.maxValue > 0 ? qsTr("max %1 %2").arg(chart.maxValue.toFixed(1)).arg(chart.unit) : ""
        color: Theme.mutedText
        font.pointSize: Theme.fontSize * 0.85
        font.features: { "tnum": 1 }
    }

    Item {
        id: plot
        anchors.top: scaleLabel.bottom
        anchors.topMargin: 4
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: labels.top
        anchors.bottomMargin: 4

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: Theme.border
            opacity: 0.5
            visible: chart.maxValue > 0
        }

        Row {
            anchors.fill: parent
            spacing: chart.gap

            Repeater {
                model: chart.points

                delegate: Item {
                    id: bar
                    required property var modelData
                    width: (plot.width - chart.gap * (chart.count - 1)) / Math.max(1, chart.count)
                    height: plot.height

                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: chart.maxValue > 0 && bar.modelData.value > 0
                                ? Math.max(2, Math.round(parent.height * bar.modelData.value / chart.maxValue))
                                : 0
                        color: bar.modelData.current || hover.hovered ? Theme.accent : Theme.chartMuted
                    }

                    HoverHandler { id: hover }
                    ToolTip.visible: hover.hovered
                    ToolTip.text: bar.modelData.detail
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.border
        }
    }

    Row {
        id: labels
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: chart.gap
        height: labelMetrics.height

        Repeater {
            model: chart.points

            delegate: Item {
                required property var modelData
                required property int index
                width: (plot.width - chart.gap * (chart.count - 1)) / Math.max(1, chart.count)
                height: labels.height

                Label {
                    // Centred under the bar, but kept inside the chart at both ends.
                    x: Math.max(-parent.x, Math.min(labels.width - parent.x - width, (parent.width - width) / 2))
                    // Label the most recent bar and every labelEvery-th before it.
                    visible: (chart.count - 1 - parent.index) % chart.labelEvery === 0
                    text: parent.modelData.label
                    color: Theme.mutedText
                    font.pointSize: Theme.fontSize * 0.85
                }
            }
        }
    }

    TextMetrics {
        id: labelMetrics
        font.pointSize: Theme.fontSize * 0.85
        text: "Sep"
    }
}
