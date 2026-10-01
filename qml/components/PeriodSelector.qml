import QtQuick

// Week / Month / Year / All.
TabSwitch {
    name: qsTr("History period")
    tabs: [
        { text: qsTr("Week"), tip: qsTr("This week (Ctrl+1)") },
        { text: qsTr("Month"), tip: qsTr("This month (Ctrl+2)") },
        { text: qsTr("Year"), tip: qsTr("This year (Ctrl+3)") },
        { text: qsTr("All"), tip: qsTr("All runs (Ctrl+4)") }
    ]
}
