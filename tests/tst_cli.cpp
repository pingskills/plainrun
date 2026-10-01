#include "database/database.h"
#include "database/migrations.h"
#include "database/runrepository.h"
#include "plainrun_config.h"
#include "services/cli.h"

#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

using namespace plainrun;

namespace {

struct Output {
    int code;
    QString out;
    QString err;
};

Output runCli(const QStringList &args, const QString &dbPath, const QDate &today = QDate(2026, 9, 30))
{
    QString out, err;
    QTextStream o(&out), e(&err);
    const int code = cli::run(QStringList{"plainrun"} + args, dbPath, o, e, today);
    o.flush();
    e.flush();
    return {code, out, err};
}

} // namespace

class TestCli : public QObject
{
    Q_OBJECT

    QTemporaryDir dir;
    QString dbPath;

private slots:
    void initTestCase()
    {
        dbPath = dir.filePath("plainrun.db");
        db::Database db;
        QVERIFY(db.open(dbPath));
        const QList<std::tuple<QDate, qint64, qint64>> runs = {
            {QDate(2026, 9, 20), 5000, 1770}, // last week (Sunday)
            {QDate(2026, 9, 23), 5000, 1680}, // last week
            {QDate(2026, 9, 28), 10000, 3480}, // this week (Monday)
            {QDate(2026, 8, 30), 7000, 2400}, // last month
            {QDate(2025, 12, 31), 4000, 1300}, // last year
        };
        for (const auto &[date, metres, seconds] : runs) {
            Run r;
            r.date = date;
            r.distanceMetres = metres;
            r.durationSeconds = seconds;
            QVERIFY(db::insertRun(db.connection(), r));
        }
    }

    void detectsCliInvocation()
    {
        const char *gui[] = {"plainrun"};
        QVERIFY(!cli::isCliInvocation(1, const_cast<char **>(gui)));
        const char *qtArgs[] = {"plainrun", "-platform", "offscreen"};
        QVERIFY(!cli::isCliInvocation(3, const_cast<char **>(qtArgs)));
        const char *week[] = {"plainrun", "--week-distance"};
        QVERIFY(cli::isCliInvocation(2, const_cast<char **>(week)));
        const char *version[] = {"plainrun", "--version"};
        QVERIFY(cli::isCliInvocation(2, const_cast<char **>(version)));
    }

    void distances()
    {
        QCOMPARE(runCli({"--week-distance"}, dbPath).out, QStringLiteral("10.00\n"));
        QCOMPARE(runCli({"--month-distance"}, dbPath).out, QStringLiteral("20.00\n"));
        QCOMPARE(runCli({"--year-distance"}, dbPath).out, QStringLiteral("27.00\n"));
        QCOMPARE(runCli({"--week-distance"}, dbPath).code, cli::ExitOk);
    }

    void lastRun()
    {
        const Output o = runCli({"--last-run"}, dbPath);
        QCOMPARE(o.code, cli::ExitOk);
        QCOMPARE(o.out, QStringLiteral("2026-09-28\t10.00\t58:00\t5:48\n"));
    }

    void versionAndHelp()
    {
        QCOMPARE(runCli({"--version"}, dbPath).out, QStringLiteral("plainrun " PLAINRUN_VERSION "\n"));
        const Output help = runCli({"--help"}, dbPath);
        QCOMPARE(help.code, cli::ExitOk);
        QVERIFY(help.out.contains("--week-distance"));
    }

    void usageErrors()
    {
        QCOMPARE(runCli({"--week-distance", "--month-distance"}, dbPath).code, cli::ExitUsage);
        QCOMPARE(runCli({"--bogus"}, dbPath).code, cli::ExitUsage);
        QCOMPARE(runCli({"--week-distance", "extra"}, dbPath).code, cli::ExitUsage);
    }

    void missingDatabaseMeansNoRunsAndIsNotCreated()
    {
        const QString missing = dir.filePath("nothing-here/plainrun.db");
        QCOMPARE(runCli({"--week-distance"}, missing).out, QStringLiteral("0.00\n"));
        QCOMPARE(runCli({"--last-run"}, missing).code, cli::ExitNoRuns);
        QVERIFY(!QFile::exists(missing));
    }

    void readsADatabaseNotYetUpgraded()
    {
        // The CLI opens read-only and never migrates, so after installing a
        // newer PlainRun it may meet a schema-1 database the GUI hasn't
        // upgraded yet.
        const QString path = dir.filePath("v1.db");
        {
            db::Database db;
            QVERIFY(db.open(path, db::Database::Mode::ReadWrite, db::migrations().mid(0, 1)));
            QSqlQuery q(db.connection());
            QVERIFY(q.exec("INSERT INTO runs (run_date, distance_metres, duration_seconds, note, created_at, "
                           "updated_at) VALUES ('2026-09-28', 5000, 1500, '', '2026-09-28T06:00:00Z', "
                           "'2026-09-28T06:00:00Z')"));
        }
        const Output last = runCli({"--last-run"}, path);
        QCOMPARE(last.code, 0);
        QVERIFY2(last.err.isEmpty(), qPrintable(last.err));
        QVERIFY(last.out.startsWith("2026-09-28\t5.00"));
        QCOMPARE(runCli({"--week-distance"}, path).out, QStringLiteral("5.00\n"));

        // And it is left at schema 1: the CLI never changes the database.
        db::ScopedConnection c(path, true);
        QSqlDatabase raw = c.db();
        QCOMPARE(db::userVersion(raw), 1);
    }

    void unreadableDatabaseIsAnError()
    {
        const QString bad = dir.filePath("bad.db");
        QFile f(bad);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(1024, 'q'));
        f.close();
        const Output o = runCli({"--week-distance"}, bad);
        QCOMPARE(o.code, cli::ExitDatabaseError);
        QVERIFY(o.out.isEmpty());
        QVERIFY(!o.err.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestCli)
#include "tst_cli.moc"
