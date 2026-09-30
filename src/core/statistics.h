#pragma once

#include "core/periods.h"
#include "core/run.h"

#include <QList>

#include <optional>

namespace plainrun {

// Aggregates are always derived from the stored runs; nothing here is persisted.
struct Totals {
    int runs = 0;
    qint64 metres = 0;
    qint64 seconds = 0;

    qint64 averageMetres() const { return runs > 0 ? (metres + runs / 2) / runs : 0; }
    // Overall average pace: total time over total distance.
    qint64 averagePaceSecondsPerKm() const;
};

Totals totalsIn(const QList<Run> &runs, const DateRange &range);

struct Bucket {
    DateRange range;
    Totals totals;
};

// The `count` most recent calendar weeks (Monday start), oldest first,
// the last one being the week containing `today`.
QList<Bucket> weeklyBuckets(const QList<Run> &runs, const QDate &today, int count);
// The `count` most recent calendar months, oldest first.
QList<Bucket> monthlyBuckets(const QList<Run> &runs, const QDate &today, int count);

struct Overview {
    std::optional<Run> lastRun;
    Totals thisWeek;
    Totals previousWeek;
    Totals thisMonth;
    Totals previousMonth;
    Totals thisYear;
};

Overview overviewFor(const QList<Run> &runs, const QDate &today);

} // namespace plainrun
