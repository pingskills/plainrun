#pragma once

#include <QDate>
#include <QLocale>
#include <QString>

#include <optional>

namespace plainrun {

// Parses a duration typed by a person. Accepted forms:
//   "28"        -> 28 minutes
//   "28:15"     -> 28 minutes 15 seconds (minutes may exceed 59, e.g. "75:00")
//   "1:02:30"   -> 1 hour 2 minutes 30 seconds
// Seconds (and minutes in the h:mm:ss form) must be two digits below 60.
std::optional<qint64> parseDurationSeconds(const QString &text);

// Parses a distance in kilometres ("5", "7.2", "7,2", "10.5 km") into whole
// metres. Converted exactly from the decimal text, rounding half up to the
// nearest metre. Does not range-check; see validation.h.
std::optional<qint64> parseDistanceMetres(const QString &text);

// Parses an ISO date (YYYY-MM-DD).
std::optional<QDate> parseIsoDate(const QString &text);

// Pace in whole seconds per kilometre, rounded to the nearest second.
// Returns 0 when it cannot be calculated.
qint64 paceSecondsPerKm(qint64 distanceMetres, qint64 durationSeconds);

// "28:15", "1:02:30"
QString formatDuration(qint64 seconds);
// "1h 27m", "45m" (rounded to the nearest minute)
QString formatDurationShort(qint64 seconds);
// "5:39"; "–" when pace is 0
QString formatPace(qint64 secondsPerKm);
// Kilometres with a fixed number of decimals, e.g. formatKm(15230, 1) -> "15.2"
QString formatKm(qint64 metres, int decimals, const QLocale &locale = QLocale::c());
QString formatIsoDate(const QDate &date);

} // namespace plainrun
