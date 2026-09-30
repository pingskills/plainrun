#include "core/statistics.h"

#include "core/runformat.h"

namespace plainrun {

qint64 Totals::averagePaceSecondsPerKm() const
{
    return paceSecondsPerKm(metres, seconds);
}

Totals totalsIn(const QList<Run> &runs, const DateRange &range)
{
    Totals t;
    for (const Run &r : runs) {
        if (!range.contains(r.date))
            continue;
        ++t.runs;
        t.metres += r.distanceMetres;
        t.seconds += r.durationSeconds;
    }
    return t;
}

QList<Bucket> weeklyBuckets(const QList<Run> &runs, const QDate &today, int count)
{
    QList<Bucket> buckets;
    buckets.reserve(count);
    const QDate thisWeek = startOfWeek(today);
    for (int i = count - 1; i >= 0; --i) {
        const DateRange range = weekContaining(thisWeek.addDays(-7 * i));
        buckets.append({range, totalsIn(runs, range)});
    }
    return buckets;
}

QList<Bucket> monthlyBuckets(const QList<Run> &runs, const QDate &today, int count)
{
    QList<Bucket> buckets;
    buckets.reserve(count);
    const QDate thisMonth(today.year(), today.month(), 1);
    for (int i = count - 1; i >= 0; --i) {
        const DateRange range = monthContaining(thisMonth.addMonths(-i));
        buckets.append({range, totalsIn(runs, range)});
    }
    return buckets;
}

Overview overviewFor(const QList<Run> &runs, const QDate &today)
{
    Overview o;
    for (const Run &r : runs) {
        if (!o.lastRun || newerThan(r, *o.lastRun))
            o.lastRun = r;
    }
    o.thisWeek = totalsIn(runs, weekContaining(today));
    o.previousWeek = totalsIn(runs, previousWeek(today));
    o.thisMonth = totalsIn(runs, monthContaining(today));
    o.previousMonth = totalsIn(runs, previousMonth(today));
    o.thisYear = totalsIn(runs, yearContaining(today));
    return o;
}

} // namespace plainrun
