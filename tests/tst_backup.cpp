#include "database/backup.h"
#include "database/database.h"
#include "database/runrepository.h"

#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

using namespace plainrun;
using namespace plainrun::db;

namespace {

void addRun(Database &db, const QDate &date, qint64 metres, qint64 seconds, const QString &note = {})
{
    Run r;
    r.date = date;
    r.distanceMetres = metres;
    r.durationSeconds = seconds;
    r.note = note;
    QVERIFY(insertRun(db.connection(), r));
}

QStringList snapshot(Database &db)
{
    QStringList rows;
    for (const Run &r : allRuns(db.connection()))
        rows << QStringLiteral("%1|%2|%3|%4|%5").arg(r.id).arg(r.date.toString(Qt::ISODate))
                    .arg(r.distanceMetres).arg(r.durationSeconds).arg(r.note);
    return rows;
}

} // namespace

class TestBackup : public QObject
{
    Q_OBJECT

private slots:
    void backupAndRestoreRoundTrip()
    {
        QTemporaryDir dir;
        const QString live = dir.filePath("data/plainrun.db");
        QVERIFY(QDir().mkpath(dir.filePath("data")));
        const QString backup = dir.filePath("plainrun-backup.db");

        Database db;
        QVERIFY(db.open(live));
        addRun(db, QDate(2026, 9, 20), 5000, 1770, "first");
        addRun(db, QDate(2026, 9, 27), 10000, 3480, "Ünïcödé, \"quoted\"");
        const QStringList atBackup = snapshot(db);

        QString error;
        QVERIFY2(writeBackup(db, backup, &error), qPrintable(error));
        QVERIFY(!QFile::exists(backup + ".partial"));

        const BackupInfo info = inspectBackup(backup);
        QVERIFY2(info.valid, qPrintable(info.error));
        QCOMPARE(info.runCount, 2);
        QCOMPARE(info.latestRun, QDate(2026, 9, 27));

        // Change data after the backup.
        addRun(db, QDate(2026, 9, 29), 3000, 900, "after backup");
        QCOMPARE(snapshot(db).size(), 3);

        QString safety;
        QVERIFY2(restoreBackup(db, backup, &safety, &error), qPrintable(error));
        QVERIFY(db.isOpen());
        QCOMPARE(snapshot(db), atBackup);

        // The replaced data was preserved and is itself a valid backup.
        QVERIFY(QFile::exists(safety));
        QCOMPARE(inspectBackup(safety).runCount, 3);
        QVERIFY(!QFile::exists(dir.filePath("data/plainrun.db.restoring")));

        // The live database is still usable after the restore.
        addRun(db, QDate(2026, 9, 30), 5000, 1500);
        QCOMPARE(snapshot(db).size(), 3);
    }

    void backupOverwritesExistingFileAtomically()
    {
        QTemporaryDir dir;
        Database db;
        QVERIFY(db.open(dir.filePath("plainrun.db")));
        addRun(db, QDate(2026, 9, 1), 5000, 1500);
        const QString target = dir.filePath("backup.db");
        QFile f(target);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("old");
        f.close();
        QString error;
        QVERIFY2(writeBackup(db, target, &error), qPrintable(error));
        QCOMPARE(inspectBackup(target).runCount, 1);
    }

    void backupToUnwritableLocationFails()
    {
        QTemporaryDir dir;
        Database db;
        QVERIFY(db.open(dir.filePath("plainrun.db")));
        QString error;
        QVERIFY(!writeBackup(db, dir.filePath("no/such/dir/backup.db"), &error));
        QVERIFY(!error.isEmpty());
    }

    void invalidBackupsAreRejected()
    {
        QTemporaryDir dir;

        QVERIFY(!inspectBackup(dir.filePath("missing.db")).valid);

        const QString empty = dir.filePath("empty.db");
        QVERIFY(QFile(empty).open(QIODevice::WriteOnly));
        QVERIFY(!inspectBackup(empty).valid);

        const QString text = dir.filePath("runs.csv");
        {
            QFile f(text);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("date,distance_km,duration\n2026-09-01,5,25:00\n");
        }
        const BackupInfo csvInfo = inspectBackup(text);
        QVERIFY(!csvInfo.valid);
        QVERIFY(csvInfo.error.contains("not a SQLite database"));

        const QString foreign = dir.filePath("foreign.db");
        {
            ScopedConnection c(foreign, false);
            QSqlQuery q(c.db());
            QVERIFY(q.exec("CREATE TABLE runs (x)"));
        }
        QVERIFY(inspectBackup(foreign).error.contains("not a PlainRun backup"));

        const QString newer = dir.filePath("newer.db");
        {
            Database db;
            QVERIFY(db.open(newer));
        }
        {
            ScopedConnection c(newer, false);
            QSqlQuery q(c.db());
            QVERIFY(q.exec("PRAGMA user_version = 99"));
        }
        QVERIFY(inspectBackup(newer).error.contains("newer version"));
    }

    void corruptBackupIsRejected()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("corrupt.db");
        {
            Database db;
            QVERIFY(db.open(path));
            for (int i = 0; i < 200; ++i)
                addRun(db, QDate(2026, 1, 1).addDays(i % 250), 5000 + i, 1500 + i, QString(200, QChar('n')));
        }
        // Damage pages after the header.
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadWrite));
        f.seek(4096 + 100);
        f.write(QByteArray(8192, '\xff'));
        f.close();

        const BackupInfo info = inspectBackup(path);
        QVERIFY(!info.valid);
    }

    void failedRestoreLeavesCurrentDataUntouched()
    {
        QTemporaryDir dir;
        Database db;
        QVERIFY(db.open(dir.filePath("plainrun.db")));
        addRun(db, QDate(2026, 9, 1), 5000, 1500, "precious");
        const QStringList before = snapshot(db);

        const QString bogus = dir.filePath("bogus.db");
        {
            QFile f(bogus);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(QByteArray(2048, 'z'));
        }
        QString safety, error;
        QVERIFY(!restoreBackup(db, bogus, &safety, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(db.isOpen());
        QCOMPARE(snapshot(db), before);
        QVERIFY(safety.isEmpty());
        // No stray files left in the data directory.
        const QStringList files = QDir(dir.path()).entryList({"plainrun*"}, QDir::Files);
        QCOMPARE(files, QStringList{"plainrun.db"});
    }
};

QTEST_GUILESS_MAIN(TestBackup)
#include "tst_backup.moc"
