import QtQuick
import QtQuick.Controls
import PlainRun.Core

// Small, quiet heading used above each block of information.
Label {
    color: Theme.mutedText
    font.pointSize: Theme.fontSize * 0.82
    font.weight: Font.DemiBold
    font.letterSpacing: 0.8
    font.capitalization: Font.AllUppercase
    Accessible.role: Accessible.Heading
}
