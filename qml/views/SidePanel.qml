import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

// Right-hand column: the selected run, two small charts and personal bests.
ScrollView {
    id: panel

    property var selectedRunId: -1
    signal editRequested(var runId)
    signal deleteRequested(var runId)

    // Re-evaluated whenever the data or the selection changes.
    readonly property var run: {
        App.totalRunCount; App.overview // dependencies
        return selectedRunId > 0 ? App.runDetails(selectedRunId) : { found: false }
    }

    contentWidth: availableWidth
    clip: true
    background: Rectangle { color: Theme.background }

    ColumnLayout {
        width: panel.availableWidth
        spacing: 0

        // --- Selected run --------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 6

            SectionLabel { text: qsTr("Run") }

            Label {
                Layout.fillWidth: true
                visible: !panel.run.found
                text: App.totalRunCount > 0 ? qsTr("Select a run to see its details.")
                                            : qsTr("Your runs will appear here.")
                color: Theme.mutedText
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                visible: panel.run.found
                text: panel.run.dateLong || ""
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
            }

            GridLayout {
                visible: panel.run.found
                columns: 3
                columnSpacing: 20
                rowSpacing: 0
                Layout.topMargin: 2

                SectionLabel { text: qsTr("Distance") }
                SectionLabel { text: qsTr("Time") }
                SectionLabel { text: qsTr("Pace") }
                Label {
                    text: panel.run.distanceText || ""
                    font.pointSize: Theme.fontSize * 1.25
                    font.features: { "tnum": 1 }
                }
                Label {
                    text: panel.run.durationText || ""
                    font.pointSize: Theme.fontSize * 1.25
                    font.features: { "tnum": 1 }
                }
                Label {
                    text: panel.run.paceText || ""
                    font.pointSize: Theme.fontSize * 1.25
                    font.features: { "tnum": 1 }
                }
            }

            TextEdit {
                Layout.fillWidth: true
                Layout.topMargin: 4
                visible: panel.run.found && (panel.run.note || "").length > 0
                text: panel.run.note || ""
                readOnly: true
                selectByMouse: true
                wrapMode: Text.Wrap
                color: Theme.text
                selectionColor: Theme.selection
                selectedTextColor: Theme.selectionText
                font: Qt.application.font
                Accessible.name: qsTr("Note")
            }

            RowLayout {
                visible: panel.run.found
                Layout.topMargin: 6
                spacing: 8
                Button {
                    text: qsTr("Edit")
                    onClicked: panel.editRequested(panel.selectedRunId)
                    ToolTip.visible: hovered
                    ToolTip.delay: 700
                    ToolTip.text: qsTr("Edit this run (Ctrl+E)")
                }
                Button {
                    text: qsTr("Delete")
                    onClicked: panel.deleteRequested(panel.selectedRunId)
                    ToolTip.visible: hovered
                    ToolTip.delay: 700
                    ToolTip.text: qsTr("Delete this run (Delete)")
                }
            }
        }

        // --- Charts ----------------------------------------------------------
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; visible: App.totalRunCount > 0 }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 8
            visible: App.totalRunCount > 0

            SectionLabel { text: qsTr("Weekly distance · 12 weeks") }
            BarChart {
                Layout.fillWidth: true
                Layout.preferredHeight: 130
                title: qsTr("Weekly distance")
                points: App.weeklyChart
                labelEvery: 4
            }

            SectionLabel {
                text: qsTr("Monthly distance · 12 months")
                Layout.topMargin: 16
            }
            BarChart {
                Layout.fillWidth: true
                Layout.preferredHeight: 130
                title: qsTr("Monthly distance")
                points: App.monthlyChart
                labelEvery: 3
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
