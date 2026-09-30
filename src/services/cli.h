#pragma once

#include <QDate>
#include <QStringList>
#include <QTextStream>

namespace plainrun::cli {

// Exit codes
inline constexpr int ExitOk = 0;
inline constexpr int ExitDatabaseError = 1;
inline constexpr int ExitUsage = 2;
inline constexpr int ExitNoRuns = 3;

// True when argv asks for a CLI command (or --help/--version), in which case
// PlainRun runs headless and never starts the GUI.
bool isCliInvocation(int argc, char **argv);

// Runs a read-only command against the database at `databasePath`.
// `arguments` includes the program name as the first element.
int run(const QStringList &arguments, const QString &databasePath, QTextStream &out,
        QTextStream &err, const QDate &today);

} // namespace plainrun::cli
