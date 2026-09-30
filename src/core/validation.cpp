#include "core/validation.h"

#include "core/runformat.h"

#include <QCoreApplication>

namespace plainrun {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("Validation", text);
}

QString dateProblem(const QDate &date, const QDate &today)
{
    if (!date.isValid())
        return tr("Enter a date as YYYY-MM-DD.");
    if (date < EarliestRunDate)
        return tr("That date is too far in the past.");
    if (today.isValid() && date > today)
        return tr("That date is in the future.");
    return {};
}

QString distanceProblem(qint64 metres)
{
    if (metres <= 0)
        return tr("Distance must be more than zero.");
    if (metres > MaxDistanceMetres)
        return tr("Distance is too large (maximum 1000 km).");
    return {};
}

QString durationProblem(qint64 seconds)
{
    if (seconds <= 0)
        return tr("Time must be more than zero.");
    if (seconds > MaxDurationSeconds)
        return tr("Time is too long (maximum 168 hours).");
    return {};
}

} // namespace

RunValidation validateRunInput(const QString &date, const QString &distance,
                               const QString &duration, const QDate &today)
{
    RunValidation v;

    if (date.trimmed().isEmpty()) {
        v.dateError = tr("Enter a date.");
    } else if (auto d = parseIsoDate(date)) {
        v.date = *d;
        v.dateError = dateProblem(v.date, today);
    } else {
        v.dateError = tr("Enter a date as YYYY-MM-DD.");
    }

    if (distance.trimmed().isEmpty()) {
        v.distanceError = tr("Enter a distance in km.");
    } else if (auto m = parseDistanceMetres(distance)) {
        v.distanceMetres = *m;
        v.distanceError = distanceProblem(v.distanceMetres);
    } else {
        v.distanceError = tr("Enter a distance in km, like 5 or 7.2.");
    }

    if (duration.trimmed().isEmpty()) {
        v.durationError = tr("Enter a time.");
    } else if (auto s = parseDurationSeconds(duration)) {
        v.durationSeconds = *s;
        v.durationError = durationProblem(v.durationSeconds);
    } else {
        v.durationError = tr("Enter a time like 28:15 or 1:02:30.");
    }

    return v;
}

QString validateRunValues(const QDate &date, qint64 distanceMetres,
                          qint64 durationSeconds, const QDate &today)
{
    QString p = dateProblem(date, today);
    if (p.isEmpty())
        p = distanceProblem(distanceMetres);
    if (p.isEmpty())
        p = durationProblem(durationSeconds);
    return p;
}

QString normaliseNote(const QString &note)
{
    return note.trimmed();
}

} // namespace plainrun
