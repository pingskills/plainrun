#pragma once

#include "core/run.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace plainrun::csv {

// Format (UTF-8, comma separated, RFC 4180 quoting, LF line endings):
//   date,distance_km,duration,pace,avg_hr_bpm,note
//   2026-09-30,5.000,28:15,5:39,148,"Easy run, around Karkarook"
// date is ISO 8601; distance_km has three decimals (exact metres);
// duration is m:ss or h:mm:ss; pace is informational and ignored on import;
// avg_hr_bpm is whole beats per minute, empty when not recorded.
inline const QStringList Header = {QStringLiteral("date"), QStringLiteral("distance_km"),
                                   QStringLiteral("duration"), QStringLiteral("pace"),
                                   QStringLiteral("avg_hr_bpm"), QStringLiteral("note")};

QString escapeField(const QString &field);

// Runs are written oldest first.
QString exportRuns(const QList<Run> &runs);

struct Table {
    QList<QStringList> rows;
    QList<int> lineNumbers; // 1-based line on which each row starts
    QString error;          // non-empty if the text is not well-formed CSV
    int errorLine = 0;
};

// RFC 4180 parser: quoted fields may contain commas, quotes ("") and newlines.
// Accepts LF or CRLF line endings and a leading UTF-8 BOM. Blank lines are skipped.
Table parse(const QString &text);

struct ImportRow {
    Run run;
    int line = 0;
};

struct ImportResult {
    QList<ImportRow> rows;
    QStringList problems; // one human-readable entry per malformed row
};

ImportResult parseRuns(const QString &text, const QDate &today);

} // namespace plainrun::csv
