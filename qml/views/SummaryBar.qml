import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

// The top strip: when did I last run, and how much this week / month / year.
Pane {
    id: bar

    readonly property var o: App.overview
    readonly property bool narrow: width < Theme.fontSize * 72

    function runsText(count, time) {
        if (count === 0)
            return qsTr("No runs")
        return count === 1 ? qsTr("1 run · %1").arg(time) : qsTr("%1 runs · %2").arg(count).arg(time)
    }

    padding: 16
    topPadding: 14
    bottomPadding: 14
    background: Rectangle { color: Theme.background }

    RowLayout {
        anchors.fill: parent
        spacing: bar.narrow ? 12 : 24

        SummaryCell {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            valueScale: bar.narrow ? 1.25 : 1.55
            label: qsTr("Last run")
            value: bar.o.hasRuns ? bar.o.lastRunDate : qsTr("No runs yet")
            detail: bar.o.hasRuns ? bar.o.lastRunAgo : ""
            secondary: bar.o.hasRuns ? bar.o.lastRunDistance + " · " + bar.o.lastRunPace : ""
        }
        SummaryCell {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            valueScale: bar.narrow ? 1.25 : 1.55
            label: qsTr("This week")
            value: bar.o.weekKm
            detail: bar.runsText(bar.o.weekRuns, bar.o.weekTime)
            secondary: bar.o.prevWeekRuns > 0 ? qsTr("Last week %1").arg(bar.o.prevWeekKm)
                                              : bar.o.hasRuns ? qsTr("No runs last week") : ""
        }
        SummaryCell {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            valueScale: bar.narrow ? 1.25 : 1.55
            label: qsTr("This month")
            value: bar.o.monthKm
            detail: bar.runsText(bar.o.monthRuns, bar.o.monthTime)
            secondary: bar.o.prevMonthRuns > 0 ? qsTr("Last month %1").arg(bar.o.prevMonthKm)
                                               : bar.o.hasRuns ? qsTr("No runs last month") : ""
        }
        SummaryCell {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            valueScale: bar.narrow ? 1.25 : 1.55
            label: qsTr("This year")
            value: bar.o.yearKm
            detail: bar.runsText(bar.o.yearRuns, bar.o.yearTime)
            secondary: bar.o.yearRuns > 0 ? qsTr("Average pace %1").arg(bar.o.yearPace) : ""
        }
    }
}
