#include "core/runformat.h"

#include <QTest>

using namespace plainrun;

class TestRunFormat : public QObject
{
    Q_OBJECT

private slots:
    void parseDuration_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<qint64>("seconds"); // -1 = invalid

        QTest::newRow("m:ss") << "28:15" << qint64(28 * 60 + 15);
        QTest::newRow("h:mm:ss") << "1:02:30" << qint64(3600 + 2 * 60 + 30);
        QTest::newRow("sub-minute") << "0:45" << qint64(45);
        QTest::newRow("minutes only") << "28" << qint64(28 * 60);
        QTest::newRow("minutes over an hour") << "75:00" << qint64(75 * 60);
        QTest::newRow("long run") << "3:59:59" << qint64(3 * 3600 + 59 * 60 + 59);
        QTest::newRow("multi-day ultra") << "26:10:05" << qint64(26 * 3600 + 10 * 60 + 5);
        QTest::newRow("whitespace") << "  28:15 " << qint64(28 * 60 + 15);
        QTest::newRow("zero") << "0:00" << qint64(0);
        QTest::newRow("empty") << "" << qint64(-1);
        QTest::newRow("seconds 60") << "28:60" << qint64(-1);
        QTest::newRow("minutes 60 in h:mm:ss") << "1:60:00" << qint64(-1);
        QTest::newRow("single-digit seconds") << "28:5" << qint64(-1);
        QTest::newRow("letters") << "abc" << qint64(-1);
        QTest::newRow("negative") << "-28:15" << qint64(-1);
        QTest::newRow("too many parts") << "1:2:3:4" << qint64(-1);
        QTest::newRow("trailing colon") << "28:" << qint64(-1);
        QTest::newRow("decimal") << "28.5" << qint64(-1);
    }
    void parseDuration()
    {
        QFETCH(QString, input);
        QFETCH(qint64, seconds);
        const auto parsed = parseDurationSeconds(input);
        if (seconds < 0) {
            QVERIFY(!parsed.has_value());
        } else {
            QVERIFY(parsed.has_value());
            QCOMPARE(*parsed, seconds);
        }
    }

    void parseDistance_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<qint64>("metres"); // -1 = invalid

        QTest::newRow("integer") << "5" << qint64(5000);
        QTest::newRow("decimal") << "7.2" << qint64(7200);
        QTest::newRow("comma decimal") << "7,2" << qint64(7200);
        QTest::newRow("with unit") << "10.5 km" << qint64(10500);
        QTest::newRow("three decimals") << "21.097" << qint64(21097);
        QTest::newRow("round half up") << "21.0975" << qint64(21098);
        QTest::newRow("round down") << "5.0004" << qint64(5000);
        QTest::newRow("leading dot") << ".5" << qint64(500);
        QTest::newRow("trailing dot") << "5." << qint64(5000);
        QTest::newRow("zero") << "0" << qint64(0);
        QTest::newRow("empty") << "" << qint64(-1);
        QTest::newRow("negative") << "-5" << qint64(-1);
        QTest::newRow("letters") << "five" << qint64(-1);
        QTest::newRow("two dots") << "5.2.1" << qint64(-1);
        QTest::newRow("dot only") << "." << qint64(-1);
    }
    void parseDistance()
    {
        QFETCH(QString, input);
        QFETCH(qint64, metres);
        const auto parsed = parseDistanceMetres(input);
        if (metres < 0) {
            QVERIFY(!parsed.has_value());
        } else {
            QVERIFY(parsed.has_value());
            QCOMPARE(*parsed, metres);
        }
    }

    void pace_data()
    {
        QTest::addColumn<qint64>("metres");
        QTest::addColumn<qint64>("seconds");
        QTest::addColumn<QString>("pace");

        QTest::newRow("spec example") << qint64(5000) << qint64(28 * 60 + 15) << "5:39";
        QTest::newRow("acceptance run 1") << qint64(5000) << qint64(30 * 60) << "6:00";
        QTest::newRow("acceptance run 2") << qint64(5000) << qint64(28 * 60) << "5:36";
        QTest::newRow("acceptance run 3") << qint64(10000) << qint64(58 * 60) << "5:48";
        QTest::newRow("edited run 1") << qint64(5000) << qint64(29 * 60 + 30) << "5:54";
        QTest::newRow("rounds half up") << qint64(2000) << qint64(601) << "5:01"; // 300.5 s/km
        QTest::newRow("over an hour") << qint64(21098) << qint64(3600 + 45 * 60) << "4:59";
        QTest::newRow("slow walk") << qint64(1000) << qint64(3700) << "61:40";
        QTest::newRow("no distance") << qint64(0) << qint64(100) << "–";
    }
    void pace()
    {
        QFETCH(qint64, metres);
        QFETCH(qint64, seconds);
        QFETCH(QString, pace);
        QCOMPARE(formatPace(paceSecondsPerKm(metres, seconds)), pace);
    }

    void formatDuration_data()
    {
        QTest::addColumn<qint64>("seconds");
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("shortText");

        QTest::newRow("sub-minute") << qint64(45) << "0:45" << "1m";
        QTest::newRow("sub-hour") << qint64(28 * 60 + 15) << "28:15" << "28m";
        QTest::newRow("exactly an hour") << qint64(3600) << "1:00:00" << "1h 00m";
        QTest::newRow("over an hour") << qint64(3600 + 2 * 60 + 30) << "1:02:30" << "1h 03m";
        QTest::newRow("week summary") << qint64(3600 + 27 * 60) << "1:27:00" << "1h 27m";
        QTest::newRow("many hours") << qint64(105 * 3600 + 20 * 60) << "105:20:00" << "105h 20m";
    }
    void formatDuration()
    {
        QFETCH(qint64, seconds);
        QFETCH(QString, text);
        QFETCH(QString, shortText);
        QCOMPARE(plainrun::formatDuration(seconds), text);
        QCOMPARE(formatDurationShort(seconds), shortText);
        // Round trip through the parser.
        QCOMPARE(parseDurationSeconds(plainrun::formatDuration(seconds)).value(), seconds);
    }

    void formatKmUsesFixedDecimals()
    {
        QCOMPARE(formatKm(15230, 1), QStringLiteral("15.2"));
        QCOMPARE(formatKm(5000, 2), QStringLiteral("5.00"));
        QCOMPARE(formatKm(21098, 3), QStringLiteral("21.098"));
        QCOMPARE(formatKm(0, 2), QStringLiteral("0.00"));
        QCOMPARE(formatKm(7200, 1, QLocale(QLocale::German)), QStringLiteral("7,2"));
    }

    void heartRate()
    {
        QCOMPARE(parseHeartRate("148").value(), 148);
        QCOMPARE(parseHeartRate(" 148 BPM").value(), 148);
        QCOMPARE(parseHeartRate("0").value(), 0); // parsed; rejected by validation
        QVERIFY(!parseHeartRate(""));
        QVERIFY(!parseHeartRate("bpm"));
        QVERIFY(!parseHeartRate("148.5"));
        QVERIFY(!parseHeartRate("1480"));
        QVERIFY(!parseHeartRate("-1"));
    }

    void beatsPerKm()
    {
        // 145 bpm at 5:30/km: 145 × 5.5 = 797.5, rounded half up.
        QCOMPARE(plainrun::beatsPerKm(5000, 5 * 330, 145), 798);
        // 10 km in 50:00 at 160 bpm: 160 × 5 = 800.
        QCOMPARE(plainrun::beatsPerKm(10000, 3000, 160), 800);
        QCOMPARE(plainrun::beatsPerKm(5000, 1650, 0), 0);
        QCOMPARE(plainrun::beatsPerKm(0, 1650, 145), 0);
        QCOMPARE(plainrun::beatsPerKm(5000, 0, 145), 0);
    }

    void isoDates()
    {
        QCOMPARE(parseIsoDate("2026-09-30").value(), QDate(2026, 9, 30));
        QCOMPARE(parseIsoDate("2024-02-29").value(), QDate(2024, 2, 29)); // leap year
        QVERIFY(!parseIsoDate("2026-02-29"));                              // not a leap year
        QVERIFY(!parseIsoDate("2026-13-01"));
        QVERIFY(!parseIsoDate("2026-9-30"));
        QVERIFY(!parseIsoDate("30/09/2026"));
        QVERIFY(!parseIsoDate(""));
    }
};

QTEST_GUILESS_MAIN(TestRunFormat)
#include "tst_runformat.moc"
