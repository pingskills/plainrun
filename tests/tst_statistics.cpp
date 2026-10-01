#include "core/periods.h"
#include "core/statistics.h"

#include <QTest>

using namespace plainrun;

namespace {

Run run(const QDate &date, qint64 metres, qint64 seconds, qint64 id = 0)
{
    Run r;
    r.id = id;
    r.date = date;
    r.distanceMetres = metres;
    r.durationSeconds = seconds;
    return r;
}

// 5 km at 5:00/km, so beats per km is exactly 5 × bpm.
Run hrRun(const QDate &date, int bpm)
{
    Run r = run(date, 5000, 1500);
    r.heartRateBpm = bpm;
    return r;
}

} // namespace

class TestStatistics : public QObject
{
    Q_OBJECT

private slots:
    void weekStartsOnMonday()
    {
        QCOMPARE(startOfWeek(QDate(2026, 9, 30)), QDate(2026, 9, 28)); // Wednesday
        QCOMPARE(startOfWeek(QDate(2026, 9, 28)), QDate(2026, 9, 28)); // Monday itself
        QCOMPARE(startOfWeek(QDate(2026, 10, 4)), QDate(2026, 9, 28)); // Sunday belongs to previous Monday
        QCOMPARE(weekContaining(QDate(2026, 10, 4)).last, QDate(2026, 10, 4));
    }

    void weekSpanningTwoMonths()
    {
        // Mon 28 Sep – Sun 4 Oct 2026
        const DateRange w = weekContaining(QDate(2026, 10, 2));
        QCOMPARE(w.first, QDate(2026, 9, 28));
        QCOMPARE(w.last, QDate(2026, 10, 4));
        const QList<Run> runs = {run(QDate(2026, 9, 29), 5000, 1500), run(QDate(2026, 10, 3), 7000, 2100)};
        QCOMPARE(totalsIn(runs, w).metres, 12000);
        QCOMPARE(totalsIn(runs, monthContaining(QDate(2026, 10, 3))).metres, 7000);
        QCOMPARE(totalsIn(runs, monthContaining(QDate(2026, 9, 1))).metres, 5000);
    }

    void decemberToJanuary()
    {
        // Mon 29 Dec 2025 – Sun 4 Jan 2026 spans the year boundary.
        const DateRange w = weekContaining(QDate(2026, 1, 2));
        QCOMPARE(w.first, QDate(2025, 12, 29));
        QCOMPARE(w.last, QDate(2026, 1, 4));

        const QDate today(2026, 1, 2);
        QCOMPARE(previousMonth(today).first, QDate(2025, 12, 1));
        QCOMPARE(previousMonth(today).last, QDate(2025, 12, 31));
        QCOMPARE(previousWeek(today).first, QDate(2025, 12, 22));

        const QList<Run> runs = {
            run(QDate(2025, 12, 30), 5000, 1500),
            run(QDate(2026, 1, 1), 10000, 3000),
            run(QDate(2025, 12, 24), 3000, 900),
        };
        const Overview o = overviewFor(runs, today);
        QCOMPARE(o.thisWeek.metres, 15000);   // both sides of New Year
        QCOMPARE(o.previousWeek.metres, 3000);
        QCOMPARE(o.thisMonth.metres, 10000);  // January only
        QCOMPARE(o.previousMonth.metres, 8000);
        QCOMPARE(o.thisYear.metres, 10000);   // 2026 only
    }

    void leapYears()
    {
        QCOMPARE(monthContaining(QDate(2024, 2, 10)).last, QDate(2024, 2, 29));
        QCOMPARE(monthContaining(QDate(2026, 2, 10)).last, QDate(2026, 2, 28));
        QCOMPARE(previousMonth(QDate(2024, 3, 31)).last, QDate(2024, 2, 29));
        QCOMPARE(previousMonth(QDate(2024, 3, 31)).first, QDate(2024, 2, 1));
        // Week containing the leap day: Mon 26 Feb – Sun 3 Mar 2024
        QCOMPARE(weekContaining(QDate(2024, 2, 29)).first, QDate(2024, 2, 26));
        QCOMPARE(weekContaining(QDate(2024, 2, 29)).last, QDate(2024, 3, 3));
    }

    void weeklyBucketsCoverTwelveWeeks()
    {
        const QDate today(2026, 9, 30);
        const QList<Run> runs = {
            run(QDate(2026, 9, 28), 5000, 1500),   // this week
            run(QDate(2026, 9, 27), 10000, 3480),  // last week (Sunday)
            run(QDate(2026, 7, 13), 4000, 1300),   // Monday of the oldest of the 12 weeks
            run(QDate(2026, 7, 12), 9000, 3000),   // the Sunday before: outside the range
        };
        const QList<Bucket> buckets = weeklyBuckets(runs, today, 12);
        QCOMPARE(buckets.size(), 12);
        QCOMPARE(buckets.last().range.first, QDate(2026, 9, 28));
        QCOMPARE(buckets.first().range.first, QDate(2026, 7, 13));
        QCOMPARE(buckets.last().totals.metres, 5000);
        QCOMPARE(buckets.at(10).totals.metres, 10000);
        QCOMPARE(buckets.first().totals.metres, 4000);
        qint64 sum = 0;
        for (const Bucket &b : buckets)
            sum += b.totals.metres;
        QCOMPARE(sum, 19000);
    }

    void monthlyBucketsAcrossYear()
    {
        const QDate today(2026, 2, 15);
        const QList<Run> runs = {run(QDate(2025, 3, 1), 5000, 1500), run(QDate(2025, 2, 28), 5000, 1500),
                                 run(QDate(2026, 2, 1), 8000, 2400)};
        const QList<Bucket> months = monthlyBuckets(runs, today, 12);
        QCOMPARE(months.size(), 12);
        QCOMPARE(months.first().range.first, QDate(2025, 3, 1));
        QCOMPARE(months.last().range.last, QDate(2026, 2, 28));
        QCOMPARE(months.first().totals.metres, 5000); // Feb 2025 excluded
        QCOMPARE(months.last().totals.metres, 8000);
    }

    void monthlyBeatsPerKmIsAMedianOfEnoughRuns()
    {
        const QDate today(2026, 9, 30);
        const QList<Run> runs = {
            // September: 4 runs, even count -> mean of the middle two (700, 750).
            hrRun(QDate(2026, 9, 2), 140), hrRun(QDate(2026, 9, 9), 150),
            hrRun(QDate(2026, 9, 16), 130), hrRun(QDate(2026, 9, 23), 190), // a race doesn't skew it
            run(QDate(2026, 9, 25), 5000, 1500),                             // no heart rate: ignored
            // August: 3 runs, odd count -> the middle one.
            hrRun(QDate(2026, 8, 1), 160), hrRun(QDate(2026, 8, 2), 150), hrRun(QDate(2026, 8, 31), 170),
            // July: only 2 with a heart rate -> not enough.
            hrRun(QDate(2026, 7, 1), 150), hrRun(QDate(2026, 7, 2), 150),
            // Outside the 12 months.
            hrRun(QDate(2025, 9, 30), 150), hrRun(QDate(2025, 9, 29), 150), hrRun(QDate(2025, 9, 28), 150),
        };
        const QList<HeartRateMonth> months = monthlyBeatsPerKm(runs, today, 12);
        QCOMPARE(months.size(), 12);
        QCOMPARE(months.first().range.first, QDate(2025, 10, 1));
        QCOMPARE(months.first().runs, 0);
        QCOMPARE(months.at(9).runs, 2);       // July
        QCOMPARE(months.at(9).beatsPerKm, 0);
        QCOMPARE(months.at(10).runs, 3);      // August
        QCOMPARE(months.at(10).beatsPerKm, 800);
        QCOMPARE(months.last().runs, 4);      // September
        QCOMPARE(months.last().beatsPerKm, 725);
    }

    void totalsAndAverages()
    {
        const QList<Run> runs = {
            run(QDate(2026, 9, 20), 5000, 30 * 60),
            run(QDate(2026, 9, 23), 5000, 28 * 60),
            run(QDate(2026, 9, 27), 10000, 58 * 60),
        };
        const Totals t = totalsIn(runs, {});
        QCOMPARE(t.runs, 3);
        QCOMPARE(t.metres, 20000);
        QCOMPARE(t.seconds, 116 * 60);
        QCOMPARE(t.averageMetres(), 6667);
        QCOMPARE(t.averagePaceSecondsPerKm(), 348); // 116 min / 20 km = 5:48
        QCOMPARE(Totals{}.averagePaceSecondsPerKm(), 0);
        QCOMPARE(Totals{}.averageMetres(), 0);
    }

    void lastRunIsNewestDateThenNewestEntry()
    {
        const QList<Run> runs = {run(QDate(2026, 9, 20), 5000, 1800, 1), run(QDate(2026, 9, 27), 5000, 1800, 2),
                                 run(QDate(2026, 9, 27), 3000, 900, 3), run(QDate(2026, 9, 23), 5000, 1800, 4)};
        const Overview o = overviewFor(runs, QDate(2026, 9, 30));
        QVERIFY(o.lastRun.has_value());
        QCOMPARE(o.lastRun->id, 3);
        QVERIFY(!overviewFor({}, QDate(2026, 9, 30)).lastRun.has_value());
    }

    void periodRanges()
    {
        const QDate today(2026, 9, 30);
        QCOMPARE(rangeFor(Period::Week, today), weekContaining(today));
        QCOMPARE(rangeFor(Period::Month, today).first, QDate(2026, 9, 1));
        QCOMPARE(rangeFor(Period::Year, today).last, QDate(2026, 12, 31));
        QVERIFY(rangeFor(Period::All, today).contains(QDate(1990, 1, 1)));
    }
};

QTEST_GUILESS_MAIN(TestStatistics)
#include "tst_statistics.moc"
