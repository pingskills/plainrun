import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PlainRun
import PlainRun.Core

// Add / edit a run. Distance has focus on open because the date usually
// defaults correctly. Enter or Ctrl+S saves, Escape cancels.
Dialog {
    id: editor

    property var runId: -1
    property bool attempted: false
    readonly property var check: App.validateRun(dateField.text, distanceField.text, durationField.text)

    signal saved(var runId)

    function openForNew() {
        runId = -1
        dateField.text = App.todayIso()
        distanceField.text = ""
        durationField.text = ""
        noteField.text = ""
        attempted = false
        saveError.text = ""
        open()
    }

    function openForEdit(id) {
        const r = App.runDetails(id)
        if (!r.found)
            return
        runId = id
        dateField.text = r.dateIso
        distanceField.text = r.distanceInput
        durationField.text = r.durationText
        noteField.text = r.note
        attempted = false
        saveError.text = ""
        open()
    }

    function save() {
        attempted = true
        const result = App.saveRun(runId, dateField.text, distanceField.text, durationField.text, noteField.text)
        if (result.ok) {
            close()
            saved(result.id)
            return
        }
        const hasFieldError = result.dateError || result.distanceError || result.durationError
        saveError.text = hasFieldError ? "" : result.message
        if (result.dateError) dateField.forceActiveFocus()
        else if (result.distanceError) distanceField.forceActiveFocus()
        else if (result.durationError) durationField.forceActiveFocus()
    }

    // Show a field's problem once the user has tried to save, or has typed
    // something and moved on. Never nag while the field is being typed into.
    function problem(field, message) {
        if (!message)
            return ""
        if (attempted)
            return message
        return field.text.length > 0 && !field.activeFocus ? message : ""
    }

    title: runId > 0 ? qsTr("Edit Run") : qsTr("Add Run")
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(Math.round(Theme.fontSize * 44), parent.width - 32)
    modal: true
    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, Theme.dark ? 0.45 : 0.25) }
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: 20

    onOpened: {
        distanceField.forceActiveFocus()
        distanceField.selectAll()
    }

    Shortcut {
        sequences: [StandardKey.Save]
        enabled: editor.opened
        onActivated: editor.save()
    }

    contentItem: GridLayout {
        columns: 2
        columnSpacing: 14
        rowSpacing: 4

        // Date
        Label {
            text: qsTr("Date")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: dateField
                objectName: "dateField"
                Layout.preferredWidth: Math.round(Theme.fontSize * 11)
                inputMethodHints: Qt.ImhDate
                placeholderText: "YYYY-MM-DD"
                selectByMouse: true
                font.features: { "tnum": 1 }
                Accessible.name: qsTr("Date, year-month-day. Up and Down change the day.")
                onAccepted: editor.save()
                Keys.onUpPressed: text = App.shiftDate(text, 1)
                Keys.onDownPressed: text = App.shiftDate(text, -1)
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_PageUp) { text = App.shiftDate(text, 7); event.accepted = true }
                    else if (event.key === Qt.Key_PageDown) { text = App.shiftDate(text, -7); event.accepted = true }
                }
                ToolTip.visible: activeFocus && !editor.problem(dateField, editor.check.dateError)
                ToolTip.delay: 1200
                ToolTip.text: qsTr("Up/Down: ±1 day · Page Up/Down: ±1 week")
            }
            Button {
                text: qsTr("Today")
                flat: true
                focusPolicy: Qt.TabFocus
                onClicked: dateField.text = App.todayIso()
            }
            Button {
                text: qsTr("Yesterday")
                flat: true
                focusPolicy: Qt.TabFocus
                onClicked: dateField.text = App.shiftDate(App.todayIso(), -1)
            }
            Item { Layout.fillWidth: true }
        }
        Item { width: 1; height: 1 }
        Label {
            Layout.fillWidth: true
            Layout.bottomMargin: 6
            readonly property string err: editor.problem(dateField, editor.check.dateError)
            text: err || editor.check.dateLong || " "
            color: err ? Theme.danger : Theme.mutedText
            font.pointSize: Theme.fontSize * 0.9
            elide: Text.ElideRight
        }

        // Distance
        Label {
            text: qsTr("Distance")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
        RowLayout {
            spacing: 6
            TextField {
                id: distanceField
                objectName: "distanceField"
                Layout.preferredWidth: Math.round(Theme.fontSize * 11)
                placeholderText: "5"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                selectByMouse: true
                font.features: { "tnum": 1 }
                Accessible.name: qsTr("Distance in kilometres")
                onAccepted: editor.save()
            }
            Label { text: qsTr("km"); color: Theme.mutedText }
        }
        Item { width: 1; height: 1 }
        Label {
            Layout.fillWidth: true
            Layout.bottomMargin: 6
            readonly property string err: editor.problem(distanceField, editor.check.distanceError)
            text: err || " "
            color: Theme.danger
            font.pointSize: Theme.fontSize * 0.9
            wrapMode: Text.Wrap
        }

        // Duration
        Label {
            text: qsTr("Time")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
        RowLayout {
            spacing: 6
            TextField {
                id: durationField
                objectName: "durationField"
                Layout.preferredWidth: Math.round(Theme.fontSize * 11)
                placeholderText: "28:15"
                selectByMouse: true
                font.features: { "tnum": 1 }
                Accessible.name: qsTr("Time, as minutes:seconds or hours:minutes:seconds")
                onAccepted: editor.save()
            }
            Label { text: qsTr("m:ss or h:mm:ss"); color: Theme.mutedText }
        }
        Item { width: 1; height: 1 }
        Label {
            Layout.fillWidth: true
            Layout.bottomMargin: 6
            readonly property string err: editor.problem(durationField, editor.check.durationError)
            text: err || " "
            color: Theme.danger
            font.pointSize: Theme.fontSize * 0.9
            wrapMode: Text.Wrap
        }

        // Pace (calculated)
        Label {
            text: qsTr("Pace")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        }
        Label {
            Layout.fillWidth: true
            text: editor.check.paceText || "–"
            color: editor.check.paceText ? Theme.text : Theme.mutedText
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
            Accessible.name: qsTr("Calculated pace %1").arg(text)
        }
        Item { width: 1; height: 10 }
        Item { width: 1; height: 10 }

        // Note
        Label {
            text: qsTr("Note")
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.topMargin: 6
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(Theme.fontSize * 7)
            TextArea {
                id: noteField
                placeholderText: qsTr("Optional")
                wrapMode: Text.Wrap
                selectByMouse: true
                Accessible.name: qsTr("Note, optional")
                KeyNavigation.priority: KeyNavigation.BeforeItem
                KeyNavigation.tab: saveButton
            }
        }

        Item { width: 1; height: 1 }
        Label {
            id: saveError
            Layout.fillWidth: true
            visible: text.length > 0
            color: Theme.danger
            wrapMode: Text.Wrap
        }
    }

    footer: RowLayout {
        spacing: 8
        Label {
            Layout.leftMargin: 20
            Layout.bottomMargin: 16
            Layout.fillWidth: true
            text: qsTr("Ctrl+S save · Esc cancel")
            color: Theme.mutedText
            font.pointSize: Theme.fontSize * 0.85
            elide: Text.ElideRight
        }
        Button {
            text: qsTr("Cancel")
            Layout.bottomMargin: 16
            onClicked: editor.reject()
            Keys.onReturnPressed: clicked()
            Keys.onEnterPressed: clicked()
        }
        AccentButton {
            id: saveButton
            text: qsTr("Save Run")
            Layout.rightMargin: 20
            Layout.bottomMargin: 16
            onClicked: editor.save()
            Keys.onReturnPressed: clicked()
            Keys.onEnterPressed: clicked()
        }
    }
}
