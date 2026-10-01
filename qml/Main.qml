import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

ApplicationWindow {
    id: window

    property var selectedRunId: -1
    // Tiling window managers ignore minimumWidth; below this width the trends
    // panel becomes a separate view (Runs | Trends) so the history and Add Run
    // always fit.
    readonly property bool compact: width < Theme.fontSize * 62
    // Short windows get a one-line summary so the list keeps most of the height.
    readonly property bool shortWindow: height < Theme.fontSize * 48
    // Compact layout only: show Trends in place of the run list.
    property bool showTrends: false
    readonly property bool trendsShown: compact && showTrends

    width: 1080
    height: 720
    minimumWidth: 760
    minimumHeight: 500
    visible: true
    title: "PlainRun"
    color: Theme.background

    palette.window: Theme.background
    palette.windowText: Theme.text
    palette.base: Theme.base
    palette.alternateBase: Theme.surface
    palette.text: Theme.text
    palette.button: Theme.surface
    palette.buttonText: Theme.text
    palette.brightText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentText
    palette.placeholderText: Theme.mutedText
    palette.light: Theme.surface
    palette.midlight: Theme.surface
    palette.mid: Theme.border
    palette.dark: Theme.border
    palette.shadow: Theme.border
    palette.toolTipBase: Theme.surface
    palette.toolTipText: Theme.text
    palette.link: Theme.accent
    palette.disabled.windowText: Theme.mutedText
    palette.disabled.text: Theme.mutedText
    palette.disabled.buttonText: Theme.mutedText

    Component.onCompleted: history.focusList()

    // --- Actions -------------------------------------------------------------

    function addRun() {
        if (App.ready)
            runEditor.openForNew()
    }
    function editRun(id) {
        if (App.ready && id > 0)
            runEditor.openForEdit(id)
    }
    function confirmDelete(id) {
        const r = App.runDetails(id)
        if (!r.found)
            return
        deleteConfirm.runId = id
        deleteConfirm.text = qsTr("Delete the run on %1 (%2 in %3)? This cannot be undone.")
                                 .arg(r.dateLong).arg(r.distanceText).arg(r.durationText)
        deleteConfirm.open()
    }
    function showStatus(message) {
        toast.text = message
        toast.visible = true
        toastTimer.restart()
    }
    function showError(title, message, details) {
        messageDialog.show(title, message, details)
    }
    function showRuns() {
        showTrends = false
        history.focusList()
    }
    function showTrendsView() {
        showTrends = true
        trendsPanel.forceActiveFocus()
    }
    function afterSave(id) {
        // Make sure the saved run is visible in the history.
        if (App.runs.indexOfId(id) < 0) {
            App.search = ""
            App.period = 3
        }
        selectedRunId = id
        history.syncCurrent()
        showRuns()
    }
    function documentsFolder() {
        return StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Add Run…")
                shortcut: StandardKey.New
                enabled: App.ready
                onTriggered: window.addRun()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Backup Data…")
                shortcut: "Ctrl+B"
                enabled: App.ready
                onTriggered: {
                    backupDialog.currentFolder = window.documentsFolder()
                    backupDialog.selectedFile = window.documentsFolder() + "/" + App.defaultBackupFileName()
                    backupDialog.open()
                }
            }
            Action {
                text: qsTr("&Restore Data…")
                enabled: App.ready
                onTriggered: restoreDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Export CSV…")
                enabled: App.ready
                onTriggered: {
                    exportDialog.currentFolder = window.documentsFolder()
                    exportDialog.selectedFile = window.documentsFolder() + "/" + App.defaultExportFileName()
                    exportDialog.open()
                }
            }
            Action {
                text: qsTr("&Import CSV…")
                enabled: App.ready
                onTriggered: importDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: qsTr("&Run")
            Action {
                text: qsTr("&Edit…")
                shortcut: "Ctrl+E"
                enabled: App.ready && window.selectedRunId > 0
                onTriggered: window.editRun(window.selectedRunId)
            }
            Action {
                text: qsTr("&Delete…")
                enabled: App.ready && window.selectedRunId > 0
                onTriggered: window.confirmDelete(window.selectedRunId)
            }
        }
        Menu {
            title: qsTr("&View")
            ActionGroup { id: periodGroup }
            Action {
                text: qsTr("This &Week")
                shortcut: "Ctrl+1"
                checkable: true
                checked: App.period === 0
                ActionGroup.group: periodGroup
                onTriggered: { App.period = 0; window.showRuns() }
            }
            Action {
                text: qsTr("This &Month")
                shortcut: "Ctrl+2"
                checkable: true
                checked: App.period === 1
                ActionGroup.group: periodGroup
                onTriggered: { App.period = 1; window.showRuns() }
            }
            Action {
                text: qsTr("This &Year")
                shortcut: "Ctrl+3"
                checkable: true
                checked: App.period === 2
                ActionGroup.group: periodGroup
                onTriggered: { App.period = 2; window.showRuns() }
            }
            Action {
                text: qsTr("&All Runs")
                shortcut: "Ctrl+4"
                checkable: true
                checked: App.period === 3
                ActionGroup.group: periodGroup
                onTriggered: { App.period = 3; window.showRuns() }
            }
            MenuSeparator {}
            // Narrow windows show the runs or the trends; wide ones show both.
            Action {
                text: window.trendsShown ? qsTr("Show &Runs") : qsTr("Show &Trends")
                shortcut: "Ctrl+T"
                enabled: App.ready && window.compact
                onTriggered: window.trendsShown ? window.showRuns() : window.showTrendsView()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Search Notes")
                shortcut: StandardKey.Find
                enabled: App.ready
                onTriggered: {
                    window.showTrends = false
                    history.focusSearch()
                }
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About PlainRun")
                onTriggered: aboutDialog.open()
            }
        }
    }

    // --- Content -------------------------------------------------------------

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: App.ready

        SummaryBar {
            Layout.fillWidth: true
            condensed: window.shortWindow
        }
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            HistoryView {
                id: history
                visible: !window.trendsShown
                compact: window.compact
                shortWindow: window.shortWindow
                onTrendsRequested: window.showTrendsView()
                Layout.fillWidth: true
                Layout.fillHeight: true
                selectedRunId: window.selectedRunId
                onSelected: id => window.selectedRunId = id
                onEditRequested: id => window.editRun(id)
                onDeleteRequested: id => window.confirmDelete(id)
                onAddRequested: window.addRun()
            }
            Rectangle {
                Layout.fillHeight: true
                width: 1
                color: Theme.border
                visible: !window.compact
            }
            SidePanel {
                id: trendsPanel
                objectName: "trendsPanel"
                visible: !window.compact || window.trendsShown
                compact: window.compact
                Layout.fillHeight: true
                Layout.fillWidth: window.compact
                Layout.preferredWidth: window.compact ? -1 : Math.round(Math.min(Theme.fontSize * 42, Math.max(Theme.fontSize * 25, window.width * 0.34)))
                onRunsRequested: window.showRuns()
                onAddRequested: window.addRun()
            }
        }
    }

    StartupErrorView {
        anchors.fill: parent
        visible: !App.ready
    }

    // Transient confirmation line (backup written, import finished, ...).
    Rectangle {
        id: toast
        property alias text: toastLabel.text
        visible: false
        z: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        width: Math.min(toastLabel.implicitWidth + 32, parent.width - 48)
        height: toastLabel.implicitHeight + 16
        color: Theme.surface
        border.color: Theme.border
        radius: 3
        Label {
            id: toastLabel
            anchors.fill: parent
            anchors.margins: 8
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            wrapMode: Text.Wrap
            verticalAlignment: Text.AlignVCenter
            Accessible.role: Accessible.AlertMessage
        }
        TapHandler { onTapped: toast.visible = false }
        Timer {
            id: toastTimer
            interval: 6000
            onTriggered: toast.visible = false
        }
    }

    // --- Dialogs -------------------------------------------------------------

    RunEditor {
        id: runEditor
        objectName: "runEditor"
        onSaved: id => window.afterSave(id)
        onClosed: history.focusList()
    }

    ConfirmDialog {
        id: deleteConfirm
        objectName: "deleteConfirm"
        property var runId: -1
        title: qsTr("Delete Run")
        confirmText: qsTr("Delete")
        destructive: true
        onConfirmed: {
            if (App.deleteRun(runId)) {
                if (window.selectedRunId === runId)
                    window.selectedRunId = -1
                window.showStatus(qsTr("Run deleted."))
            } else {
                window.showError(qsTr("Delete Run"), qsTr("The run could not be deleted."))
            }
        }
        onClosed: history.focusList()
    }

    ConfirmDialog {
        id: restoreConfirm
        property url file
        title: qsTr("Restore Data")
        confirmText: qsTr("Restore")
        destructive: true
        onConfirmed: {
            const r = App.restoreFrom(file)
            if (r.ok) {
                window.selectedRunId = -1
                window.showStatus(r.message)
            } else {
                window.showError(qsTr("Restore Data"), r.message)
            }
        }
    }

    MessageDialog { id: messageDialog }
    AboutDialog { id: aboutDialog }

    Dialogs.FileDialog {
        id: backupDialog
        title: qsTr("Backup PlainRun Data")
        fileMode: Dialogs.FileDialog.SaveFile
        defaultSuffix: "db"
        nameFilters: [qsTr("PlainRun backups (*.db)"), qsTr("All files (*)")]
        onAccepted: {
            const r = App.backupTo(selectedFile)
            if (r.ok)
                window.showStatus(r.message)
            else
                window.showError(qsTr("Backup Data"), r.message)
        }
    }

    Dialogs.FileDialog {
        id: restoreDialog
        title: qsTr("Restore PlainRun Data")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [qsTr("PlainRun backups (*.db)"), qsTr("All files (*)")]
        onAccepted: {
            const info = App.inspectBackup(selectedFile)
            if (!info.valid) {
                window.showError(qsTr("Restore Data"), qsTr("This file cannot be restored. %1").arg(info.error))
                return
            }
            const contains = info.runCount === 1 ? qsTr("1 run") : qsTr("%1 runs").arg(info.runCount)
            const latest = info.latestRun ? qsTr(", latest %1").arg(info.latestRun) : ""
            const current = info.currentRunCount === 1 ? qsTr("1 run") : qsTr("%1 runs").arg(info.currentRunCount)
            restoreConfirm.file = selectedFile
            restoreConfirm.text = qsTr("The backup contains %1%2.\n\nIt will replace the %3 currently in PlainRun. "
                                       + "Your current data is saved to a separate file in the data folder first.")
                                      .arg(contains).arg(latest).arg(current)
            restoreConfirm.open()
        }
    }

    Dialogs.FileDialog {
        id: exportDialog
        title: qsTr("Export Runs as CSV")
        fileMode: Dialogs.FileDialog.SaveFile
        defaultSuffix: "csv"
        nameFilters: [qsTr("CSV files (*.csv)"), qsTr("All files (*)")]
        onAccepted: {
            const r = App.exportCsv(selectedFile)
            if (r.ok)
                window.showStatus(r.message)
            else
                window.showError(qsTr("Export CSV"), r.message)
        }
    }

    Dialogs.FileDialog {
        id: importDialog
        title: qsTr("Import Runs from CSV")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [qsTr("CSV files (*.csv)"), qsTr("All files (*)")]
        onAccepted: {
            const r = App.importCsv(selectedFile)
            if (r.ok)
                window.showStatus(r.message)
            else
                window.showError(qsTr("Import CSV"), r.message, r.details)
        }
    }
}
