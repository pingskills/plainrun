import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

// Trends: distance charts, beats per km and personal bests. A right-hand
// column in the wide layout, a full view (Runs | Trends) in the compact one.
ScrollView {
    id: panel

    property bool compact: false
    signal runsRequested()
    signal addRequested()

    // A line needs two points: months with enough runs with a heart rate.
    readonly property int heartRateMonths: App.heartRateChart.filter(p => p.value > 0).length

    contentWidth: availableWidth
    clip: true
    background: Rectangle { color: Theme.background }

    ColumnLayout {
        width: panel.availableWidth
        spacing: 0

        // --- Compact header: switch back to Runs ---------------------------
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 16
            Layout.topMargin: 8
            visible: panel.compact
            spacing: 12
            TabSwitch {
                objectName: "trendsViewSwitch"
                name: qsTr("View")
                current: 1
                tabs: [{ text: qsTr("Runs"), tip: qsTr("Runs (Ctrl+T)") },
                       { text: qsTr("Trends"), tip: qsTr("Trends (Ctrl+T)") }]
                onChosen: i => { if (i === 0) panel.runsRequested() }
            }
            Item { Layout.fillWidth: true }
            AccentButton {
                text: qsTr("Add Run")
                onClicked: panel.addRequested()
                Accessible.name: qsTr("Add run")
                ToolTip.visible: hovered
                ToolTip.delay: 700
                ToolTip.text: qsTr("Add a run (Ctrl+N)")
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.margins: 16
            visible: App.totalRunCount === 0
            text: qsTr("Weekly and monthly distance and your personal bests will appear here.")
            color: Theme.mutedText
            wrapMode: Text.Wrap
        }

        // --- Charts ----------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 8
            visible: App.totalRunCount > 0

            SectionLabel { text: qsTr("Weekly distance · 12 weeks") }
            BarChart {
                Layout.fillWidth: true
                Layout.preferredHeight: 104
                title: qsTr("Weekly distance")
                points: App.weeklyChart
                labelEvery: 4
            }

            SectionLabel {
                text: qsTr("Monthly distance · 12 months")
                Layout.topMargin: 10
            }
            BarChart {
                Layout.fillWidth: true
                Layout.preferredHeight: 104
                title: qsTr("Monthly distance")
                points: App.monthlyChart
                labelEvery: 3
            }

            // Hidden entirely until a heart rate has been recorded.
            SectionLabel {
                visible: App.heartRateRunCount > 0
                text: qsTr("Beats per km · monthly median")
                Layout.topMargin: 10
            }
            TrendChart {
                visible: App.heartRateRunCount > 0 && panel.heartRateMonths >= 2
                Layout.fillWidth: true
                Layout.preferredHeight: 92
                title: qsTr("Beats per km")
                unit: qsTr("beats/km")
                points: App.heartRateChart
                labelEvery: 3
            }
            Label {
                Layout.fillWidth: true
                visible: App.heartRateRunCount > 0 && panel.heartRateMonths < 2
                text: qsTr("Heart rate × minutes per km. Shows once two months each have three runs with a heart rate.")
                color: Theme.mutedText
                wrapMode: Text.Wrap
            }
        }

        // --- Personal bests --------------------------------------------------
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; visible: App.totalRunCount > 0 }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 6
            visible: App.totalRunCount > 0

            SectionLabel { text: qsTr("Personal bests") }

            Label {
                Layout.fillWidth: true
                visible: App.personalBests.length === 0
                text: qsTr("Runs within 1% short or 3% long of 1 km, 5 km, 10 km, a half or a full marathon will appear here.")
                color: Theme.mutedText
                wrapMode: Text.Wrap
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 16
                rowSpacing: 4
                visible: App.personalBests.length > 0

                Repeater {
                    model: App.personalBests
                    delegate: Label {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData.label
                        Layout.row: index
                        Layout.column: 0
                        required property int index
                    }
                }
                Repeater {
                    model: App.personalBests
                    delegate: Label {
                        required property var modelData
                        required property int index
                        Layout.row: index
                        Layout.column: 1
                        Layout.alignment: Qt.AlignRight
                        text: modelData.timeText
                        font.weight: Font.DemiBold
                        font.features: { "tnum": 1 }
                        ToolTip.visible: pbHover.hovered
                        ToolTip.text: qsTr("%1 at %2").arg(modelData.distanceText).arg(modelData.paceText)
                        HoverHandler { id: pbHover }
                    }
                }
                Repeater {
                    model: App.personalBests
                    delegate: Label {
                        required property var modelData
                        required property int index
                        Layout.row: index
                        Layout.column: 2
                        Layout.alignment: Qt.AlignRight
                        text: modelData.dateText
                        color: Theme.mutedText
                        font.features: { "tnum": 1 }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
