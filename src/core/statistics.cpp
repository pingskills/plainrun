#include "core/statistics.h"

#include "core/runformat.h"

#include <algorithm>

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

QList<HeartRateMonth> monthlyBeatsPerKm(const QList<Run> &runs, const QDate &today, int count)
{
    QList<HeartRateMonth> months;
    months.reserve(count);
    const QDate thisMonth(today.year(), today.month(), 1);
    for (int i = count - 1; i >= 0; --i) {
        HeartRateMonth m;
        m.range = monthContaining(thisMonth.addMonths(-i));
        QList<qint64> values;
        for (const Run &r : runs) {
            if (r.heartRateBpm > 0 && m.range.contains(r.date))
                values.append(beatsPerKm(r.distanceMetres, r.durationSeconds, r.heartRateBpm));
        }
        m.runs = static_cast<int>(values.size());
        if (m.runs >= MinHeartRateRunsPerMonth) {
            std::sort(values.begin(), values.end());
            const qsizetype mid = values.size() / 2;
            // Even count: the mean of the middle two, rounded half up.
            m.beatsPerKm = values.size() % 2 ? values.at(mid) : (values.at(mid - 1) + values.at(mid) + 1) / 2;
        }
        months.append(m);
    }
    return months;
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
