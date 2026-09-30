#include "core/csv.h"

#include "core/runformat.h"
#include "core/validation.h"

#include <QCoreApplication>

#include <algorithm>

namespace plainrun::csv {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("Csv", text);
}

} // namespace

QString escapeField(const QString &field)
{
    const bool needsQuotes = field.contains(QLatin1Char(',')) || field.contains(QLatin1Char('"'))
        || field.contains(QLatin1Char('\n')) || field.contains(QLatin1Char('\r'))
        || (!field.isEmpty() && (field.front().isSpace() || field.back().isSpace()));
    if (!needsQuotes)
        return field;
    QString quoted = field;
    quoted.replace(QLatin1Char('"'), QLatin1String("\"\""));
    return QLatin1Char('"') + quoted + QLatin1Char('"');
}

QString exportRuns(const QList<Run> &runs)
{
    QList<Run> sorted = runs;
    std::sort(sorted.begin(), sorted.end(), [](const Run &a, const Run &b) { return newerThan(b, a); });

    QString out = Header.join(QLatin1Char(',')) + QLatin1Char('\n');
    for (const Run &r : sorted) {
        const QStringList fields = {
            formatIsoDate(r.date),
            formatKm(r.distanceMetres, 3),
            formatDuration(r.durationSeconds),
            formatPace(paceSecondsPerKm(r.distanceMetres, r.durationSeconds)),
            r.note,
        };
        QStringList escaped;
        for (const QString &f : fields)
            escaped << escapeField(f);
        out += escaped.join(QLatin1Char(',')) + QLatin1Char('\n');
    }
    return out;
}

Table parse(const QString &input)
{
    Table table;
    QString text = input;
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);

    QStringList row;
    QString field;
    bool inQuotes = false;
    bool fieldWasQuoted = false;
    bool rowHasContent = false;
    int line = 1;
    int rowStartLine = 1;

    auto endField = [&] {
        row << field;
        field.clear();
        fieldWasQuoted = false;
    };
    auto endRow = [&] {
        endField();
        const bool blank = !rowHasContent && row.size() == 1 && row.front().isEmpty();
        if (!blank) {
            table.rows << row;
            table.lineNumbers << rowStartLine;
        }
        row.clear();
        rowHasContent = false;
    };

    const qsizetype n = text.size();
    for (qsizetype i = 0; i < n; ++i) {
        const QChar c = text.at(i);
        if (inQuotes) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < n && text.at(i + 1) == QLatin1Char('"')) {
                    field += QLatin1Char('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                if (c == QLatin1Char('\n'))
                    ++line;
                field += c;
            }
            continue;
        }

        if (c == QLatin1Char('"')) {
            if (!field.isEmpty() || fieldWasQuoted) {
                table.error = tr("Unexpected quote character inside a field.");
                table.errorLine = line;
                return table;
            }
            inQuotes = true;
            fieldWasQuoted = true;
            rowHasContent = true;
        } else if (c == QLatin1Char(',')) {
            endField();
            rowHasContent = true;
        } else if (c == QLatin1Char('\r')) {
            // Tolerate CRLF: the following LF ends the row.
            if (i + 1 < n && text.at(i + 1) == QLatin1Char('\n'))
                continue;
            endRow();
            ++line;
            rowStartLine = line;
        } else if (c == QLatin1Char('\n')) {
            endRow();
            ++line;
            rowStartLine = line;
        } else {
            if (fieldWasQuoted) {
                table.error = tr("Unexpected text after a closing quote.");
                table.errorLine = line;
                return table;
            }
            field += c;
            rowHasContent = true;
        }
    }

    if (inQuotes) {
        table.error = tr("A quoted field is never closed.");
        table.errorLine = rowStartLine;
        return table;
    }
    if (rowHasContent || !field.isEmpty())
        endRow();
    return table;
}

ImportResult parseRuns(const QString &text, const QDate &today)
{
    ImportResult result;
    const Table table = parse(text);
    if (!table.error.isEmpty()) {
        result.problems << tr("Line %1: %2").arg(table.errorLine).arg(table.error);
        return result;
    }
    if (table.rows.isEmpty()) {
        result.problems << tr("The file is empty.");
        return result;
    }

    // Locate columns by header name so column order and extra columns don't matter.
    const QStringList header = table.rows.front();
    auto column = [&](const QString &name) {
        for (qsizetype i = 0; i < header.size(); ++i) {
            if (header.at(i).trimmed().compare(name, Qt::CaseInsensitive) == 0)
                return static_cast<int>(i);
        }
        return -1;
    };
    const int dateCol = column(QStringLiteral("date"));
    const int distCol = column(QStringLiteral("distance_km"));
    const int durCol = column(QStringLiteral("duration"));
    const int noteCol = column(QStringLiteral("note"));
    if (dateCol < 0 || distCol < 0 || durCol < 0) {
        result.problems << tr("Line 1: expected a header with date, distance_km and duration columns.");
        return result;
    }

    for (qsizetype r = 1; r < table.rows.size(); ++r) {
        const QStringList &row = table.rows.at(r);
        const int line = table.lineNumbers.at(r);
        if (row.size() != header.size()) {
            result.problems << tr("Line %1: expected %2 fields but found %3.")
                                   .arg(line).arg(header.size()).arg(row.size());
            continue;
        }

        const auto date = parseIsoDate(row.at(dateCol));
        if (!date) {
            result.problems << tr("Line %1: invalid date “%2”.").arg(line).arg(row.at(dateCol));
            continue;
        }
        const auto metres = parseDistanceMetres(row.at(distCol));
        if (!metres) {
            result.problems << tr("Line %1: invalid distance “%2”.").arg(line).arg(row.at(distCol));
            continue;
        }
        const auto seconds = parseDurationSeconds(row.at(durCol));
        if (!seconds) {
            result.problems << tr("Line %1: invalid duration “%2”.").arg(line).arg(row.at(durCol));
            continue;
        }
        const QString problem = validateRunValues(*date, *metres, *seconds, today);
        if (!problem.isEmpty()) {
            result.problems << tr("Line %1: %2").arg(line).arg(problem);
            continue;
        }

        Run run;
        run.date = *date;
        run.distanceMetres = *metres;
        run.durationSeconds = *seconds;
        run.note = noteCol >= 0 ? normaliseNote(row.at(noteCol)) : QString();
        result.rows.append({run, line});
    }
    return result;
}

} // namespace plainrun::csv
