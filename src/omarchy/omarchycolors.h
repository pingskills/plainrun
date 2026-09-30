#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

// Omarchy integration, part 1: locating and parsing the active theme's
// colors.toml. Pure Qt Core so it can be tested without a display. Nothing in
// PlainRun requires Omarchy; if these files are absent the caller falls back
// to normal Qt/system styling.
namespace plainrun::omarchy {

// Candidate locations of the active theme's colors.toml, most likely first:
//   $XDG_STATE_HOME/omarchy/current/theme/colors.toml   (Omarchy 4)
//   $XDG_CONFIG_HOME/omarchy/current/theme/colors.toml  (earlier releases)
QStringList colorsFileCandidates();

// The first candidate that exists, or an empty string.
QString activeColorsFile();

// Parses the flat `key = "value"` subset of TOML used by colors.toml.
// Comments, blank lines, tables and malformed lines are ignored.
QHash<QString, QString> parseColorsToml(const QString &text);

// Set PLAINRUN_NO_OMARCHY=1 to ignore Omarchy even when present.
bool disabledByEnvironment();

} // namespace plainrun::omarchy
