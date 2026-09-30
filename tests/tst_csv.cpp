#include "core/csv.h"

#include <QTest>

using namespace plainrun;

namespace {

Run run(const QDate &date, qint64 metres, qint64 seconds, const QString &note)
{
    Run r;
    r.date = date;
    r.distanceMetres = metres;
    r.durationSeconds = seconds;
    r.note = note;
    return r;
}

const QDate Today(2026, 9, 30);

} // namespace

class TestCsv : public QObject
{
    Q_OBJECT

private slots:
    void escaping()
    {
        QCOMPARE(csv::escapeField("plain"), QStringLiteral("plain"));
        QCOMPARE(csv::escapeField("a, b"), QStringLiteral("\"a, b\""));
        QCOMPARE(csv::escapeField("say \"hi\""), QStringLiteral("\"say \"\"hi\"\"\""));
        QCOMPARE(csv::escapeField("two\nlines"), QStringLiteral("\"two\nlines\""));
        QCOMPARE(csv::escapeField(" padded"), QStringLiteral("\" padded\""));
        QCOMPARE(csv::escapeField(""), QString());
    }

    void exportFormat()
    {
        const QString out = csv::exportRuns({run(QDate(2026, 9, 30), 5000, 28 * 60 + 15, "Easy run, around Karkarook"),
                                             run(QDate(2026, 9, 27), 21098, 3600 + 45 * 60, "")});
        const QStringList lines = out.split('\n');
        QCOMPARE(lines.at(0), QStringLiteral("date,distance_km,duration,pace,note"));
        QCOMPARE(lines.at(1), QStringLiteral("2026-09-27,21.098,1:45:00,4:59,")); // oldest first
        QCOMPARE(lines.at(2), QStringLiteral("2026-09-30,5.000,28:15,5:39,\"Easy run, around Karkarook\""));
        QVERIFY(out.endsWith('\n'));
    }

    void roundTripPreservesTrickyNotes()
    {
        const QList<Run> original = {
            run(QDate(2026, 1, 1), 5000, 1500, QStringLiteral("Quotes \"here\" and, commas")),
            run(QDate(2026, 1, 2), 7200, 2492, QStringLiteral("Line one\nLine two")),
            run(QDate(2026, 1, 3), 10000, 3000, QStringLiteral("Ünïcödé — 東京 🏃‍♀️")),
            run(QDate(2024, 2, 29), 1000, 240, QString()),
        };
        const csv::ImportResult parsed = csv::parseRuns(csv::exportRuns(original), Today);
        QVERIFY2(parsed.problems.isEmpty(), qPrintable(parsed.problems.join("; ")));
        QCOMPARE(parsed.rows.size(), 4);
        QCOMPARE(parsed.rows.at(0).run.date, QDate(2024, 2, 29));
        QCOMPARE(parsed.rows.at(1).run.note, QStringLiteral("Quotes \"here\" and, commas"));
        QCOMPARE(parsed.rows.at(2).run.note, QStringLiteral("Line one\nLine two"));
        QCOMPARE(parsed.rows.at(3).run.note, QStringLiteral("Ünïcödé — 東京 🏃‍♀️"));
        QCOMPARE(parsed.rows.at(2).run.distanceMetres, 7200);
        QCOMPARE(parsed.rows.at(2).run.durationSeconds, 2492);
    }

    void parserHandlesCrlfBomAndBlankLines()
    {
        const QString text = QString(QChar(0xFEFF)) + "date,distance_km,duration\r\n\r\n2026-09-01,5,25:00\r\n";
        const csv::ImportResult parsed = csv::parseRuns(text, Today);
        QVERIFY(parsed.problems.isEmpty());
        QCOMPARE(parsed.rows.size(), 1);
        QCOMPARE(parsed.rows.first().line, 3);
    }

    void columnsFoundByHeaderName()
    {
        const QString text = "note,duration,date,distance_km,extra\n\"hilly\",1:02:30,2026-09-01,12.5,x\n";
        const csv::ImportResult parsed = csv::parseRuns(text, Today);
        QVERIFY(parsed.problems.isEmpty());
        QCOMPARE(parsed.rows.first().run.durationSeconds, 3750);
        QCOMPARE(parsed.rows.first().run.distanceMetres, 12500);
        QCOMPARE(parsed.rows.first().run.note, QStringLiteral("hilly"));
    }

    void malformedRowsAreReportedWithLineNumbers()
    {
        const QString text = "date,distance_km,duration,pace,note\n"
                             "2026-09-01,5,25:00,,ok\n"
                             "2026-13-01,5,25:00,,bad date\n"
                             "2026-09-02,zero,25:00,,bad distance\n"
                             "2026-09-03,5,25:99,,bad duration\n"
                             "2026-09-04,0,25:00,,zero distance\n"
                             "2026-09-05,5,25:00\n"
                             "2027-01-01,5,25:00,,future\n";
        const csv::ImportResult parsed = csv::parseRuns(text, Today);
        QCOMPARE(parsed.rows.size(), 1);
        QCOMPARE(parsed.problems.size(), 6);
        QVERIFY(parsed.problems.at(0).startsWith("Line 3:"));
        QVERIFY(parsed.problems.at(1).startsWith("Line 4:"));
        QVERIFY(parsed.problems.at(5).startsWith("Line 8:"));
    }

    void structuralErrors()
    {
        QVERIFY(!csv::parse("a,\"unterminated\n").error.isEmpty());
        QVERIFY(!csv::parse("a,b\"c\n").error.isEmpty());
        QVERIFY(!csv::parse("a,\"b\"c\n").error.isEmpty());
        QCOMPARE(csv::parseRuns("", Today).problems.size(), 1);
        QCOMPARE(csv::parseRuns("foo,bar\n1,2\n", Today).problems.size(), 1); // missing columns
    }

    void emptyTrailingFieldIsKept()
    {
        const csv::Table t = csv::parse("a,b,\n");
        QCOMPARE(t.rows.size(), 1);
        QCOMPARE(t.rows.first().size(), 3);
    }
};

QTEST_GUILESS_MAIN(TestCsv)
#include "tst_csv.moc"
