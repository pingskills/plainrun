#pragma once

#include <QString>

namespace plainrun {

inline constexpr auto DatabaseFileName = "plainrun.db";

// Per-user data directory from QStandardPaths::AppDataLocation,
// normally $XDG_DATA_HOME/plainrun (i.e. ~/.local/share/plainrun).
// Requires QCoreApplication::applicationName() to be "plainrun".
QString dataDirectory();

// dataDirectory() + "/plainrun.db"
QString databasePath();

// Creates the data directory (and parents) if needed.
bool ensureDataDirectory(QString *error);

} // namespace plainrun
