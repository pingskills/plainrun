#include "core/personalbests.h"

#include <QTest>

using namespace plainrun;

namespace {

Run run(qint64 id, const QDate &date, qint64 metres, qint64 seconds)
{
    Run r;
    r.id = id;
    r.date = date;
    r.distanceMetres = metres;
    r.durationSeconds = seconds;
    return r;
}

} // namespace

class TestPersonalBests : public QObject
{
    Q_OBJECT

private slots:
    void tolerance_data()
    {
        QTest::addColumn<qint64>("metres");
        QTest::addColumn<double>("target");
        QTest::addColumn<bool>("qualifies");

        QTest::newRow("exact 5k") << qint64(5000) << 5000.0 << true;
        QTest::newRow("GPS long 5.10") << qint64(5100) << 5000.0 << true;
        QTest::newRow("upper bound 5.15") << qint64(5150) << 5000.0 << true;
        QTest::newRow("5.16 too long") << qint64(5160) << 5000.0 << false;
        QTest::newRow("5.4 is not a 5k") << qint64(5400) << 5000.0 << false;
        QTest::newRow("lower bound 4.95") << qint64(4950) << 5000.0 << true;
        QTest::newRow("4.94 too short") << qint64(4940) << 5000.0 << false;
        QTest::newRow("half marathon 21.1") << qint64(21100) << 21097.5 << true;
        QTest::newRow("half marathon 20.8 short") << qint64(20800) << 21097.5 << false;
        QTest::newRow("marathon 42.4") << qint64(42400) << 42195.0 << true;
        QTest::newRow("1k exact") << qint64(1000) << 1000.0 << true;
        QTest::newRow("1.2k") << qint64(1200) << 1000.0 << false;
    }
    void tolerance()
    {
        QFETCH(qint64, metres);
        QFETCH(double, target);
        QFETCH(bool, qualifies);
        QCOMPARE(qualifiesFor(metres, target), qualifies);
    }

    void fastestQualifyingRunWins()
    {
        const QList<Run> runs = {
            run(1, QDate(2026, 5, 12), 5000, 25 * 60 + 12),
            run(2, QDate(2026, 6, 1), 5400, 24 * 60),       // faster but 5.4 km: not a 5k
            run(3, QDate(2026, 7, 1), 5050, 25 * 60 + 30),  // qualifies, slower
            run(4, QDate(2026, 8, 3), 10000, 54 * 60 + 30),
            run(5, QDate(2026, 8, 20), 7000, 35 * 60),      // no recognised distance
        };
        const QList<PersonalBest> pbs = personalBests(runs);
        QCOMPARE(pbs.size(), 2);
        QCOMPARE(pbs.at(0).target.metres, 5000.0);
        QCOMPARE(pbs.at(0).run.id, 1);
        QCOMPARE(pbs.at(1).target.metres, 10000.0);
        QCOMPARE(pbs.at(1).run.id, 4);
    }

    void tieGoesToEarlierRun()
    {
        const QList<Run> runs = {run(1, QDate(2026, 8, 1), 5000, 1500), run(2, QDate(2026, 5, 1), 5000, 1500)};
        const QList<PersonalBest> pbs = personalBests(runs);
        QCOMPARE(pbs.size(), 1);
        QCOMPARE(pbs.first().run.id, 2);
    }

    void noDataNoBests()
    {
        QVERIFY(personalBests({}).isEmpty());
        QVERIFY(personalBests({run(1, QDate(2026, 1, 1), 3000, 900)}).isEmpty());
    }

    void recognisedDistances()
    {
        QCOMPARE(pbTargets().size(), 5);
        QCOMPARE(pbTargets().at(3).metres, 21097.5);
        QCOMPARE(pbTargets().at(4).metres, 42195.0);
    }
};

QTEST_GUILESS_MAIN(TestPersonalBests)
#include "tst_personalbests.moc"
