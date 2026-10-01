#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

namespace plainrun {

// One recorded run. Distance is stored in whole metres and duration in whole
// seconds; everything else (pace, totals) is derived when needed. Average heart
// rate is optional: 0 means it was not recorded.
struct Run {
    qint64 id = 0;
    QDate date;
    qint64 distanceMetres = 0;
    qint64 durationSeconds = 0;
    QString note;
    int heartRateBpm = 0;
    QDateTime createdAt;
    QDateTime updatedAt;
};

// Identity used to detect duplicate runs during CSV import. Heart rate is left
// out so that importing a file with heart rates does not duplicate runs
// already recorded without one.
inline QString duplicateKey(const Run &run)
{
    return run.date.toString(Qt::ISODate) + QLatin1Char('|')
        + QString::number(run.distanceMetres) + QLatin1Char('|')
        + QString::number(run.durationSeconds) + QLatin1Char('|') + run.note;
}

// Newest first: later date first, then most recently created (higher id).
inline bool newerThan(const Run &a, const Run &b)
{
    if (a.date != b.date)
        return a.date > b.date;
    return a.id > b.id;
}

} // namespace plainrun
