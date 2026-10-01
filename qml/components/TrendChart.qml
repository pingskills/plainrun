import QtQuick
import QtQuick.Controls
import PlainRun.Core

// A quiet line chart for values whose change matters more than their size
// (e.g. beats per km). The scale spans the data's range, not zero to max.
// points: [{ label, value, valueText, detail, current }]; value 0 = no data.
Item {
    id: chart

    property string title
    property var points: []
    property int labelEvery: 3
    property string unit

    readonly property var present: points.filter(p => p.value > 0)
    readonly property real lo: present.length ? Math.min(...present.map(p => p.value)) : 0
    readonly property real hi: present.length ? Math.max(...present.map(p => p.value)) : 1
    readonly property real span: Math.max(1, hi - lo)
    readonly property int count: points.length

    function xAt(i) { return plot.width * (i + 0.5) / Math.max(1, count) }
    function yAt(v) { return 4 + (plot.height - 8) * (1 - (v - lo) / span) }

    implicitHeight: 130
    Accessible.role: Accessible.Chart
    Accessible.name: title + ": " + present.map(p => p.label + " " + p.valueText).join(", ")

    Label {
        id: scaleLabel
        anchors.top: parent.top
        anchors.right: parent.right
        text: chart.present.length ? qsTr("%1–%2 %3").arg(chart.lo.toFixed(0)).arg(chart.hi.toFixed(0)).arg(chart.unit) : ""
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
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.border
        }

        Canvas {
            id: line
            anchors.fill: parent
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Connections { target: chart; function onPointsChanged() { line.requestPaint() } }
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = Theme.chartMuted
                ctx.lineWidth = 2
                ctx.beginPath()
                let started = false
                for (let i = 0; i < chart.count; ++i) {
                    const p = chart.points[i]
                    if (p.value <= 0) { started = false; continue }
                    const x = chart.xAt(i), y = chart.yAt(p.value)
                    if (started) ctx.lineTo(x, y); else ctx.moveTo(x, y)
                    started = true
                }
                ctx.stroke()
            }
        }

        Repeater {
            model: chart.points
            delegate: Rectangle {
                required property var modelData
                required property int index
                visible: modelData.value > 0
                width: 7; height: 7; radius: 3.5
                x: chart.xAt(index) - width / 2
                y: chart.yAt(modelData.value) - height / 2
                color: modelData.current || dotHover.hovered ? Theme.accent : Theme.chartMuted
                HoverHandler { id: dotHover; margin: 6 }
                ToolTip.visible: dotHover.hovered
                ToolTip.text: modelData.detail
            }
        }
    }

    Item {
        id: labels
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: labelMetrics.height

        Repeater {
            model: chart.points
            delegate: Label {
                required property var modelData
                required property int index
                visible: (chart.count - 1 - index) % chart.labelEvery === 0
                x: Math.max(0, Math.min(labels.width - width, chart.xAt(index) - width / 2))
                text: modelData.label
                color: Theme.mutedText
                font.pointSize: Theme.fontSize * 0.85
            }
        }
    }

    TextMetrics {
        id: labelMetrics
        font.pointSize: Theme.fontSize * 0.85
        text: "Sep"
    }
}
