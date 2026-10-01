import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

// Period tabs, note search, period statistics and the list of runs.
Pane {
    id: history

    property var selectedRunId: -1
    property bool compact: false
    property bool shortWindow: false
    // The selected run's note, heart rate and actions sit in a strip below the list.
    readonly property var selectedRun: {
        App.totalRunCount; App.overview // dependencies: any save or delete
        return selectedRunId > 0 ? App.runDetails(selectedRunId) : { found: false }
    }
    readonly property string selectedNote: selectedRun.note || ""
    signal selected(var runId)
    signal editRequested(var runId)
    signal deleteRequested(var runId)
    signal addRequested()
    signal trendsRequested()

    readonly property var stats: App.periodStats
    readonly property int numberColumnWidth: Math.round(Math.max(Theme.fontSize * 6.5, Math.min(Theme.fontSize * 8.5, width / 7)))

    function focusList() { list.forceActiveFocus() }
    // In the compact layout the search field collapses to a button until used.
    property bool searchOpen: false
    function focusSearch() { searchOpen = true; search.forceActiveFocus(); search.selectAll() }

    // Keep the list's current row in step with the selected run.
    function syncCurrent() {
        const idx = App.runs.indexOfId(history.selectedRunId)
        list.currentIndex = idx
        if (idx >= 0)
            list.positionViewAtIndex(idx, ListView.Contain)
    }
    onSelectedRunIdChanged: syncCurrent()

    Connections {
        target: App.runs
        function onModelReset() { history.syncCurrent() }
    }

    padding: 0
    background: Rectangle { color: Theme.background }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 16
            Layout.topMargin: 8
            spacing: 12

            TabSwitch {
                objectName: "runsViewSwitch"
                visible: history.compact
                name: qsTr("View")
                current: 0
                tabs: [{ text: qsTr("Runs"), tip: qsTr("Runs (Ctrl+T)") },
                       { text: qsTr("Trends"), tip: qsTr("Trends (Ctrl+T)") }]
                onChosen: i => { if (i === 1) history.trendsRequested() }
            }
            Rectangle {
                visible: history.compact
                width: 1
                Layout.preferredHeight: Math.round(Theme.fontSize * 1.8)
                color: Theme.border
            }

            PeriodSelector {
                current: App.period
                onChosen: period => App.period = period
            }

            AbstractButton {
                id: searchButton
                objectName: "searchButton"
                visible: history.compact && !history.searchOpen && App.search.length === 0
                implicitWidth: Math.round(Theme.fontSize * 2.6)
                implicitHeight: implicitWidth
                onClicked: history.focusSearch()
                Accessible.name: qsTr("Search notes")
                ToolTip.visible: hovered
                ToolTip.delay: 700
                ToolTip.text: qsTr("Search notes (Ctrl+F)")
                background: Rectangle { color: searchButton.hovered ? Theme.surface : "transparent" }
                contentItem: Item {
                    Rectangle {
                        x: parent.width * 0.28; y: parent.height * 0.26
                        width: parent.width * 0.36; height: width; radius: width / 2
                        color: "transparent"; border.width: 1.6; border.color: Theme.mutedText
                    }
                    Rectangle {
                        x: parent.width * 0.58; y: parent.height * 0.6
                        width: parent.width * 0.2; height: 1.8; rotation: 45
                        color: Theme.mutedText
                    }
                }
            }

            TextField {
                id: search
                visible: !searchButton.visible
                objectName: "searchField"
                Layout.fillWidth: true
                Layout.preferredWidth: Math.round(Theme.fontSize * 18)
                Layout.maximumWidth: Math.round(Theme.fontSize * 18)
                Layout.minimumWidth: Math.round(Theme.fontSize * 8)
                placeholderText: qsTr("Search notes")
                text: App.search
                selectByMouse: true
                Accessible.name: qsTr("Search notes")
                onTextEdited: App.search = text
                Keys.onEscapePressed: {
                    App.search = "" // clears the field through the binding above
                    list.forceActiveFocus()
                }
                Keys.onDownPressed: list.forceActiveFocus()
                onActiveFocusChanged: if (!activeFocus && text.length === 0) history.searchOpen = false
                ToolTip.visible: hovered && !activeFocus
                ToolTip.delay: 700
                ToolTip.text: qsTr("Search notes (Ctrl+F)")
            }

            Item { Layout.fillWidth: true }

            AccentButton {
                text: qsTr("Add Run")
                onClicked: history.addRequested()
                Accessible.name: qsTr("Add run")
                ToolTip.visible: hovered
                ToolTip.delay: 700
                ToolTip.text: qsTr("Add a run (Ctrl+N)")
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 12
            Layout.bottomMargin: 6
            visible: history.stats.runs > 0 && !history.shortWindow
            color: Theme.mutedText
            elide: Text.ElideRight
            font.features: { "tnum": 1 }
            text: [
                history.stats.runs === 1 ? qsTr("1 run") : qsTr("%1 runs").arg(history.stats.runs),
                history.stats.distanceText,
                history.stats.timeText,
                qsTr("avg %1").arg(history.stats.averageDistanceText),
                qsTr("avg pace %1").arg(history.stats.paceText)
            ].join("  ·  ")
            ToolTip.visible: statsHover.hovered
            ToolTip.delay: 500
            ToolTip.text: qsTr("Total time %1").arg(history.stats.timeExact)
            HoverHandler { id: statsHover }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: history.stats.runs > 0 && !history.shortWindow ? 0 : 12
            Layout.bottomMargin: 4
            spacing: 12
            visible: list.count > 0

            HeaderCell { Layout.fillWidth: true; text: qsTr("Date"); column: 0; alignment: Text.AlignLeft }
            HeaderCell { Layout.preferredWidth: history.numberColumnWidth; text: qsTr("Distance"); column: 1 }
            HeaderCell { Layout.preferredWidth: history.numberColumnWidth; text: qsTr("Time"); column: 2 }
            HeaderCell { Layout.preferredWidth: history.numberColumnWidth; text: qsTr("Pace"); column: 3 }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
            visible: list.count > 0
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            model: App.runs
            keyNavigationEnabled: true
            boundsBehavior: Flickable.StopAtBounds
            currentIndex: -1
            highlightMoveDuration: 0
            activeFocusOnTab: true
            reuseItems: true

            Accessible.role: Accessible.List
            Accessible.name: qsTr("Runs")

            ScrollBar.vertical: ScrollBar {}

            onCurrentIndexChanged: {
                const id = App.runs.idAt(currentIndex)
                if (id > 0 && id !== history.selectedRunId)
                    history.selected(id)
            }

            Keys.onReturnPressed: if (history.selectedRunId > 0) history.editRequested(history.selectedRunId)
            Keys.onEnterPressed: if (history.selectedRunId > 0) history.editRequested(history.selectedRunId)
            Keys.onDeletePressed: if (history.selectedRunId > 0) history.deleteRequested(history.selectedRunId)
            Keys.onPressed: event => {
                if (event.key === Qt.Key_Home) { currentIndex = 0; event.accepted = true }
                else if (event.key === Qt.Key_End) { currentIndex = count - 1; event.accepted = true }
            }

            delegate: RunRow {
                width: ListView.view.width
                numberColumnWidth: history.numberColumnWidth
                selected: runId === history.selectedRunId
                onActivated: {
                    list.forceActiveFocus()
                    history.selected(runId)
                }
                onEdit: history.editRequested(runId)
            }

            // Visible keyboard focus for the list as a whole.
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.width: 1
                border.color: Theme.accent
                visible: list.activeFocus && list.currentIndex < 0 && list.count > 0
            }

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 48, 360)
                visible: list.count === 0
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    font.pointSize: Theme.fontSize * 1.15
                    text: App.totalRunCount === 0 ? qsTr("No runs yet.")
                        : App.search.length > 0 ? qsTr("No runs match “%1”.").arg(App.search)
                        : [qsTr("No runs this week."), qsTr("No runs this month."),
                           qsTr("No runs this year."), qsTr("No runs yet.")][App.period]
                }
                AccentButton {
                    Layout.alignment: Qt.AlignHCenter
                    visible: App.totalRunCount === 0
                    text: qsTr("Add Run")
                    onClicked: history.addRequested()
                }
            }
        }
    
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
            visible: history.selectedRun.found
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            visible: history.selectedRun.found
            spacing: 12
            Label {
                Layout.fillWidth: true
                text: history.selectedNote.length > 0 ? history.selectedNote : qsTr("No note")
                color: Theme.mutedText
                font.italic: history.selectedNote.length === 0
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                Accessible.name: history.selectedNote.length > 0 ? qsTr("Note: %1").arg(history.selectedNote)
                                                                 : qsTr("No note")
            }
            Label {
                objectName: "selectedHeartRate"
                visible: text.length > 0
                text: history.selectedRun.heartRateText
                      ? history.selectedRun.heartRateText + " · " + history.selectedRun.beatsPerKmText : ""
                color: Theme.mutedText
                font.features: { "tnum": 1 }
                ToolTip.visible: hrHover.hovered
                ToolTip.delay: 500
                ToolTip.text: qsTr("Average heart rate · heartbeats per kilometre")
                HoverHandler { id: hrHover }
            }
            Button {
                text: qsTr("Edit")
                onClicked: history.editRequested(history.selectedRunId)
                ToolTip.visible: hovered
                ToolTip.delay: 700
                ToolTip.text: qsTr("Edit this run (Ctrl+E)")
            }
            Button {
                text: qsTr("Delete")
                onClicked: history.deleteRequested(history.selectedRunId)
                ToolTip.visible: hovered
                ToolTip.delay: 700
                ToolTip.text: qsTr("Delete this run (Delete)")
            }
        }
    }
}
