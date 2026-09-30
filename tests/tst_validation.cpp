#include "core/validation.h"

#include <QTest>

using namespace plainrun;

class TestValidation : public QObject
{
    Q_OBJECT

    const QDate today{2026, 9, 30};

private slots:
    void validInput()
    {
        const RunValidation v = validateRunInput("2026-09-30", "5", "28:15", today);
        QVERIFY(v.ok());
        QCOMPARE(v.date, QDate(2026, 9, 30));
        QCOMPARE(v.distanceMetres, 5000);
        QCOMPARE(v.durationSeconds, 28 * 60 + 15);
    }

    void rejectsZeroAndNegativeDistance()
    {
        QVERIFY(!validateRunInput("2026-09-30", "0", "28:15", today).distanceError.isEmpty());
        QVERIFY(!validateRunInput("2026-09-30", "0.0004", "28:15", today).distanceError.isEmpty());
        QVERIFY(!validateRunInput("2026-09-30", "-5", "28:15", today).distanceError.isEmpty());
        QVERIFY(!validateRunInput("2026-09-30", "", "28:15", today).distanceError.isEmpty());
        QVERIFY(!validateRunInput("2026-09-30", "1000.001", "28:15", today).distanceError.isEmpty());
        QVERIFY(validateRunInput("2026-09-30", "1000", "99:00:00", today).ok());
    }

    void rejectsZeroAndMalformedDuration()
    {
        QVERIFY(!validateRunInput("2026-09-30", "5", "0:00", today).durationError.isEmpty());
        QVERIFY(!validateRunInput("2026-09-30", "5", "0", today).durationError.isEmpty());
        QVERIFY(!validateRunInput("2026-09-30", "5", "28:75", today).durationError.isEmpty());
        QVERIFY(!validateRunInput("2026-09-30", "5", "fast", today).durationError.isEmpty());
        QVERIFY(!validateRunInput("2026-09-30", "5", "169:00:00", today).durationError.isEmpty());
    }

    void rejectsInvalidAndFutureDates()
    {
        QVERIFY(!validateRunInput("2026-02-30", "5", "28:15", today).dateError.isEmpty());
        QVERIFY(!validateRunInput("yesterday", "5", "28:15", today).dateError.isEmpty());
        QVERIFY(!validateRunInput("", "5", "28:15", today).dateError.isEmpty());
        QVERIFY(!validateRunInput("2026-10-01", "5", "28:15", today).dateError.isEmpty());
        QVERIFY(!validateRunInput("1899-12-31", "5", "28:15", today).dateError.isEmpty());
        QVERIFY(validateRunInput("2024-02-29", "5", "28:15", today).ok());
    }

    void reportsEveryProblemAtOnce()
    {
        const RunValidation v = validateRunInput("nope", "x", "y", today);
        QVERIFY(!v.ok());
        QVERIFY(!v.dateError.isEmpty());
        QVERIFY(!v.distanceError.isEmpty());
        QVERIFY(!v.durationError.isEmpty());
    }

    void valuesValidation()
    {
        QVERIFY(validateRunValues(QDate(2026, 9, 1), 5000, 1500, today).isEmpty());
        QVERIFY(!validateRunValues(QDate(2026, 9, 1), 0, 1500, today).isEmpty());
        QVERIFY(!validateRunValues(QDate(2026, 9, 1), 5000, 0, today).isEmpty());
        QVERIFY(!validateRunValues(QDate(), 5000, 1500, today).isEmpty());
    }

    void noteIsTrimmed()
    {
        QCOMPARE(normaliseNote("  Easy run \n"), QStringLiteral("Easy run"));
    }
};

QTEST_GUILESS_MAIN(TestValidation)
#include "tst_validation.moc"
