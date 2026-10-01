#include "database/database.h"
#include "database/migrations.h"
#include "database/runrepository.h"

#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

using namespace plainrun;
using namespace plainrun::db;

namespace {

Run makeRun(const QDate &date, qint64 metres, qint64 seconds, const QString &note = {})
{
    Run r;
    r.date = date;
    r.distanceMetres = metres;
    r.durationSeconds = seconds;
    r.note = note;
    return r;
}

// Executes SQL on a raw connection (for setting up odd fixtures).
void rawExec(const QString &path, const QStringList &statements)
{
    ScopedConnection c(path, false);
    QVERIFY(c.isOpen());
    QSqlQuery q(c.db());
    for (const QString &s : statements)
        QVERIFY2(q.exec(s), qPrintable(s));
}

} // namespace

class TestDatabase : public QObject
{
    Q_OBJECT

    QTemporaryDir dir;
    QString path(const QString &name) const { return dir.filePath(name); }

private slots:
    void freshDatabaseIsCreatedAndMigrated()
    {
        const QString p = path("fresh.db");
        QVERIFY(!QFile::exists(p));
        Database db;
        QVERIFY2(db.open(p), qPrintable(db.lastError()));
        QVERIFY(QFile::exists(p));
        QCOMPARE(QFileInfo(p).permissions() & 0x7077 /* owner/group/other */,
                 QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QSqlDatabase c = db.connection();
        const HeaderInfo h = readHeader(c);
        QCOMPARE(h.applicationId, Database::ApplicationId);
        QCOMPARE(h.userVersion, latestSchemaVersion());
        QVERIFY(allRuns(c).isEmpty());
    }

    void crud()
    {
        Database db;
        QVERIFY(db.open(path("crud.db")));
        QSqlDatabase c = db.connection();

        Run a = makeRun(QDate(2026, 9, 20), 5000, 1800, QStringLiteral("Ünïcödé note — 🏃"));
        QVERIFY(insertRun(c, a));
        QVERIFY(a.id > 0);
        QVERIFY(a.createdAt.isValid());

        Run b = makeRun(QDate(2026, 9, 27), 10000, 3480);
        QVERIFY(insertRun(c, b));

        QList<Run> runs = allRuns(c);
        QCOMPARE(runs.size(), 2);
        QCOMPARE(runs.first().id, b.id); // newest first
        QCOMPARE(runs.last().note, QStringLiteral("Ünïcödé note — 🏃"));

        a.durationSeconds = 1770;
        QVERIFY(updateRun(c, a));
        QCOMPARE(findRun(c, a.id)->durationSeconds, 1770);

        QVERIFY(deleteRun(c, b.id));
        QVERIFY(!findRun(c, b.id));
        QCOMPARE(allRuns(c).size(), 1);

        Run missing = a;
        missing.id = 9999;
        QString error;
        QVERIFY(!updateRun(c, missing, &error));
        QVERIFY(!error.isEmpty());
    }

    void heartRateRoundTripsAndNoneIsNull()
    {
        Database db;
        QVERIFY(db.open(path("heartrate.db")));
        QSqlDatabase c = db.connection();
        Run with = makeRun(QDate(2026, 9, 20), 5000, 1650);
        with.heartRateBpm = 145;
        Run without = makeRun(QDate(2026, 9, 21), 5000, 1650);
        QVERIFY(insertRun(c, with));
        QVERIFY(insertRun(c, without));
        QCOMPARE(findRun(c, with.id)->heartRateBpm, 145);
        QCOMPARE(findRun(c, without.id)->heartRateBpm, 0);

        QSqlQuery q(c);
        QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM runs WHERE avg_heart_rate_bpm IS NULL")) && q.next());
        QCOMPARE(q.value(0).toInt(), 1);

        with.heartRateBpm = 0;
        QVERIFY(updateRun(c, with));
        QCOMPARE(findRun(c, with.id)->heartRateBpm, 0);

        Run tooLow = makeRun(QDate(2026, 9, 22), 5000, 1650);
        tooLow.heartRateBpm = 20;
        QVERIFY(!insertRun(c, tooLow));
    }

    void version1DatabaseUpgradesWithRunsIntact()
    {
        const QString p = path("v1.db");
        {
            // Create the database exactly as PlainRun 0.1 did, with a run in it.
            Database db;
            QVERIFY(db.open(p, Database::Mode::ReadWrite, migrations().mid(0, 1)));
            QSqlDatabase c = db.connection();
            QCOMPARE(userVersion(c), 1);
        }
        rawExec(p, {QStringLiteral("INSERT INTO runs (run_date, distance_metres, duration_seconds, note, "
                                   "created_at, updated_at) VALUES ('2026-09-01', 5000, 1500, 'from 0.1', "
                                   "'2026-09-01T06:00:00Z', '2026-09-01T06:00:00Z')")});
        Database db;
        QVERIFY2(db.open(p), qPrintable(db.lastError()));
        QSqlDatabase c = db.connection();
        QCOMPARE(userVersion(c), 2);
        const QList<Run> runs = allRuns(c);
        QCOMPARE(runs.size(), 1);
        QCOMPARE(runs.first().note, QStringLiteral("from 0.1"));
        QCOMPARE(runs.first().heartRateBpm, 0);
        Run edited = runs.first();
        edited.heartRateBpm = 150;
        QVERIFY(updateRun(c, edited));
        QCOMPARE(findRun(c, edited.id)->heartRateBpm, 150);
    }

    void constraintsRejectBadRows()
    {
        Database db;
        QVERIFY(db.open(path("constraints.db")));
        Run zero = makeRun(QDate(2026, 9, 1), 0, 100);
        QVERIFY(!insertRun(db.connection(), zero));
        Run negative = makeRun(QDate(2026, 9, 1), 100, -5);
        QVERIFY(!insertRun(db.connection(), negative));
    }

    void batchInsertIsAllOrNothing()
    {
        Database db;
        QVERIFY(db.open(path("batch.db")));
        QList<Run> runs = {makeRun(QDate(2026, 9, 1), 5000, 1500), makeRun(QDate(2026, 9, 2), 0, 1500)};
        QVERIFY(!insertRuns(db.connection(), runs));
        QCOMPARE(allRuns(db.connection()).size(), 0);

        runs[1].distanceMetres = 3000;
        QVERIFY(insertRuns(db.connection(), runs));
        QCOMPARE(allRuns(db.connection()).size(), 2);
    }

    void dataSurvivesReopen()
    {
        const QString p = path("reopen.db");
        {
            Database db;
            QVERIFY(db.open(p));
            Run r = makeRun(QDate(2026, 9, 1), 5000, 1500, "kept");
            QVERIFY(insertRun(db.connection(), r));
        }
        Database db;
        QVERIFY(db.open(p));
        QCOMPARE(allRuns(db.connection()).size(), 1);
        QCOMPARE(allRuns(db.connection()).first().note, QStringLiteral("kept"));
    }

    void migrationToNewVersionPreservesData()
    {
        const QString p = path("upgrade.db");
        {
            Database db;
            QVERIFY(db.open(p));
            Run r = makeRun(QDate(2026, 9, 1), 5000, 1500, "before upgrade");
            QVERIFY(insertRun(db.connection(), r));
        }

        // Simulate a future release that adds a column.
        QList<Migration> future = migrations();
        future.append({latestSchemaVersion() + 1, {QStringLiteral("ALTER TABLE runs ADD COLUMN shoe TEXT")}});

        Database db;
        QVERIFY2(db.open(p, Database::Mode::ReadWrite, future), qPrintable(db.lastError()));
        QSqlDatabase c = db.connection();
        QCOMPARE(userVersion(c), latestSchemaVersion() + 1);
        QCOMPARE(allRuns(c).size(), 1);
        QCOMPARE(allRuns(c).first().note, QStringLiteral("before upgrade"));
        QSqlQuery q(c);
        QVERIFY(q.exec("SELECT shoe FROM runs"));
    }

    void failedMigrationRollsBack()
    {
        const QString p = path("failed.db");
        {
            Database db;
            QVERIFY(db.open(p));
            Run r = makeRun(QDate(2026, 9, 1), 5000, 1500);
            QVERIFY(insertRun(db.connection(), r));
        }
        QList<Migration> broken = migrations();
        broken.append({latestSchemaVersion() + 1,
                       {QStringLiteral("CREATE TABLE partial (x INTEGER)"),
                        QStringLiteral("THIS IS NOT SQL")}});
        {
            Database db;
            QVERIFY(!db.open(p, Database::Mode::ReadWrite, broken));
            QVERIFY(!db.lastError().isEmpty());
            QVERIFY(!db.isOpen());
        }
        // Still at the old version, data intact, partial work rolled back.
        Database db;
        QVERIFY(db.open(p));
        QSqlDatabase c = db.connection();
        QCOMPARE(userVersion(c), latestSchemaVersion());
        QCOMPARE(allRuns(c).size(), 1);
        QSqlQuery q(c);
        QVERIFY(!q.exec("SELECT * FROM partial"));
    }

    void newerSchemaIsRefusedAndUntouched()
    {
        const QString p = path("newer.db");
        {
            Database db;
            QVERIFY(db.open(p));
        }
        rawExec(p, {QStringLiteral("PRAGMA user_version = 99")});
        const QByteArray before = [&] { QFile f(p); return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray(); }();

        Database db;
        QVERIFY(!db.open(p));
        QVERIFY(db.lastError().contains("newer version"));

        QFile f(p);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), before);
    }

    void foreignDatabaseIsRefused()
    {
        const QString p = path("foreign.db");
        rawExec(p, {QStringLiteral("CREATE TABLE something (x)"), QStringLiteral("INSERT INTO something VALUES (1)")});
        Database db;
        QVERIFY(!db.open(p));
        QVERIFY(db.lastError().contains("not a PlainRun database"));
        ScopedConnection c(p, true);
        QSqlQuery q(c.db());
        QVERIFY(q.exec("SELECT COUNT(*) FROM something") && q.next());
        QCOMPARE(q.value(0).toInt(), 1);
    }

    void notSqliteIsRefused()
    {
        const QString p = path("garbage.db");
        QFile f(p);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(4096, 'x'));
        f.close();
        Database db;
        QVERIFY(!db.open(p));
        QVERIFY(QFile::exists(p));
        QCOMPARE(QFileInfo(p).size(), 4096);
    }

    void readOnlyNeverCreates()
    {
        const QString p = path("missing.db");
        Database db;
        QVERIFY(!db.open(p, Database::Mode::ReadOnly));
        QVERIFY(!QFile::exists(p));
    }
};

QTEST_GUILESS_MAIN(TestDatabase)
#include "tst_database.moc"
