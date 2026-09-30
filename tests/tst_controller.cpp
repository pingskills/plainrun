// Drives AppController — the exact API the QML interface calls — through the
// acceptance scenario from the project brief, plus import/export.
#include "services/appcontroller.h"
#include "services/cli.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace plainrun;

namespace {

const QDate Today(2026, 9, 30); // Wednesday

QString cliOut(const QStringList &args, const QString &dbPath)
{
    QString out, err;
    QTextStream o(&out), e(&err);
    cli::run(QStringList{"plainrun"} + args, dbPath, o, e, Today);
    o.flush();
    return out.trimmed();
}

QStringList state(const AppController &app)
{
    QStringList rows;
    for (const Run &r : app.allRuns())
        rows << QStringLiteral("%1|%2|%3|%4").arg(r.date.toString(Qt::ISODate)).arg(r.distanceMetres)
                    .arg(r.durationSeconds).arg(r.note);
    return rows;
}

} // namespace

class TestController : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedKingdom)); }

    void acceptanceScenario()
    {
        QTemporaryDir home;
        const QString dbPath = home.filePath("share/plainrun/plainrun.db");

        // New installation: the data directory and an empty database are created.
        QVERIFY(!QFile::exists(dbPath));
        auto app = std::make_unique<AppController>(dbPath);
        app->setFixedToday(Today);
        QVERIFY2(app->initialize(), qPrintable(app->startupError()));
        QVERIFY(QFile::exists(dbPath));
        // A newly created data directory is private to the user.
        QCOMPARE(QFileInfo(QFileInfo(dbPath).absolutePath()).permissions() & 0x7077,
                 QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        QCOMPARE(app->totalRunCount(), 0);
        QCOMPARE(app->overview().value("hasRuns").toBool(), false);

        // Add runs, checking the live pace shown in the form.
        QCOMPARE(app->validateRun("2026-09-20", "5", "30:00").value("paceText").toString(), QStringLiteral("6:00/km"));
        QCOMPARE(app->validateRun("2026-09-23", "5", "28:00").value("paceText").toString(), QStringLiteral("5:36/km"));
        QCOMPARE(app->validateRun("2026-09-27", "10", "58:00").value("paceText").toString(), QStringLiteral("5:48/km"));

        QVariantMap r1 = app->saveRun(-1, "2026-09-20", "5", "30:00", "");
        QVariantMap r2 = app->saveRun(-1, "2026-09-23", "5", "28:00", "");
        QVariantMap r3 = app->saveRun(-1, "2026-09-27", "10", "58:00", "Long run");
        QVERIFY(r1.value("ok").toBool() && r2.value("ok").toBool() && r3.value("ok").toBool());
        const qint64 run1 = r1.value("id").toLongLong();
        const qint64 run2 = r2.value("id").toLongLong();

        QCOMPARE(app->totalRunCount(), 3);
        QCOMPARE(app->runs()->count(), 3); // "All" view
        QCOMPARE(app->runs()->idAt(0), r3.value("id").toLongLong()); // newest first
        QCOMPARE(app->runDetails(run1).value("paceText").toString(), QStringLiteral("6:00/km"));
        QCOMPARE(app->runDetails(run2).value("paceText").toString(), QStringLiteral("5:36/km"));

        QVariantMap stats = app->periodStats();
        QCOMPARE(stats.value("runs").toInt(), 3);
        QCOMPARE(stats.value("distanceText").toString(), QStringLiteral("20.0 km"));
        QCOMPARE(stats.value("timeExact").toString(), QStringLiteral("1:56:00"));
        QCOMPARE(stats.value("paceText").toString(), QStringLiteral("5:48/km"));
        QCOMPARE(stats.value("averageDistanceText").toString(), QStringLiteral("6.67 km"));

        // All three runs fall in the previous calendar week (Mon 21–Sun 27 Sep) or the one before.
        QVariantMap ov = app->overview();
        QCOMPARE(ov.value("weekKm").toString(), QStringLiteral("0.0 km"));
        QCOMPARE(ov.value("prevWeekKm").toString(), QStringLiteral("15.0 km"));
        QCOMPARE(ov.value("monthKm").toString(), QStringLiteral("20.0 km"));
        QCOMPARE(ov.value("lastRunDate").toString(), QStringLiteral("Sun 27 Sept"));
        QCOMPARE(ov.value("lastRunAgo").toString(), QStringLiteral("3 days ago"));

        // Edit run 1 to 29:30.
        const QVariantMap edited = app->saveRun(run1, "2026-09-20", "5", "29:30", "");
        QVERIFY(edited.value("ok").toBool());
        QCOMPARE(app->runDetails(run1).value("paceText").toString(), QStringLiteral("5:54/km"));
        QCOMPARE(app->periodStats().value("timeExact").toString(), QStringLiteral("1:55:30"));
        QCOMPARE(app->periodStats().value("paceText").toString(), QStringLiteral("5:47/km")); // 6930 s / 20 km = 346.5

        // Delete run 2.
        QVERIFY(app->deleteRun(run2));
        QCOMPARE(app->totalRunCount(), 2);
        QCOMPARE(app->periodStats().value("distanceText").toString(), QStringLiteral("15.0 km"));
        QCOMPARE(app->overview().value("prevWeekKm").toString(), QStringLiteral("10.0 km"));

        // Backup.
        const QString backup = home.filePath("plainrun-backup-2026-09-30.db");
        QVariantMap b = app->backupTo(QUrl::fromLocalFile(backup));
        QVERIFY2(b.value("ok").toBool(), qPrintable(b.value("message").toString()));
        const QStringList backedUp = state(*app);

        // More data.
        QVERIFY(app->saveRun(-1, "2026-09-29", "8", "45:00", "after backup").value("ok").toBool());
        QCOMPARE(app->totalRunCount(), 3);
        QCOMPARE(app->overview().value("weekKm").toString(), QStringLiteral("8.0 km"));

        // Restore returns exactly to the backed-up state.
        const QVariantMap info = app->inspectBackup(QUrl::fromLocalFile(backup));
        QVERIFY(info.value("valid").toBool());
        QCOMPARE(info.value("runCount").toInt(), 2);
        QCOMPARE(info.value("currentRunCount").toInt(), 3);
        QSignalSpy dataSpy(app.get(), &AppController::dataChanged);
        const QVariantMap restored = app->restoreFrom(QUrl::fromLocalFile(backup));
        QVERIFY2(restored.value("ok").toBool(), qPrintable(restored.value("message").toString()));
        QVERIFY(dataSpy.count() >= 1);
        QCOMPARE(state(*app), backedUp);
        QCOMPARE(app->overview().value("weekKm").toString(), QStringLiteral("0.0 km"));
        QVERIFY(QFile::exists(restored.value("safetyCopy").toString()));

        // Restart: data persists.
        app.reset();
        app = std::make_unique<AppController>(dbPath);
        app->setFixedToday(Today);
        QVERIFY(app->initialize());
        QCOMPARE(state(*app), backedUp);

        // CLI matches the GUI values.
        QCOMPARE(cliOut({"--week-distance"}, dbPath), QStringLiteral("0.00"));
        QCOMPARE(cliOut({"--month-distance"}, dbPath), QStringLiteral("15.00"));
        QCOMPARE(app->overview().value("monthKm").toString(), QStringLiteral("15.0 km"));
        QCOMPARE(cliOut({"--last-run"}, dbPath), QStringLiteral("2026-09-27\t10.00\t58:00\t5:48"));
    }

    void validationErrorsDoNotSave()
    {
        QTemporaryDir home;
        AppController app(home.filePath("plainrun.db"));
        app.setFixedToday(Today);
        QVERIFY(app.initialize());
        const QVariantMap r = app.saveRun(-1, "2026-09-31", "0", "28:75", "");
        QVERIFY(!r.value("ok").toBool());
        QVERIFY(!r.value("dateError").toString().isEmpty());
        QVERIFY(!r.value("distanceError").toString().isEmpty());
        QVERIFY(!r.value("durationError").toString().isEmpty());
        QCOMPARE(app.totalRunCount(), 0);
    }

    void periodsSearchAndSorting()
    {
        QTemporaryDir home;
        AppController app(home.filePath("plainrun.db"));
        app.setFixedToday(Today);
        QVERIFY(app.initialize());
        QVERIFY(app.saveRun(-1, "2026-09-29", "5", "25:00", "parkrun").value("ok").toBool());
        QVERIFY(app.saveRun(-1, "2026-09-10", "12", "1:05:00", "Long").value("ok").toBool());
        QVERIFY(app.saveRun(-1, "2026-01-05", "8", "48:00", "Snowy PARKRUN").value("ok").toBool());
        QVERIFY(app.saveRun(-1, "2025-12-31", "3", "15:00", "").value("ok").toBool());

        app.setPeriod(0);
        QCOMPARE(app.runs()->count(), 1);
        app.setPeriod(1);
        QCOMPARE(app.runs()->count(), 2);
        app.setPeriod(2);
        QCOMPARE(app.runs()->count(), 3);
        app.setPeriod(3);
        QCOMPARE(app.runs()->count(), 4);

        app.setSearch("parkrun");
        QCOMPARE(app.runs()->count(), 2); // case-insensitive
        QCOMPARE(app.periodStats().value("distanceText").toString(), QStringLiteral("13.0 km"));
        app.setSearch("");

        app.sortBy(AppController::SortDistance); // longest first
        QCOMPARE(app.runs()->data(app.runs()->index(0), RunListModel::DistanceTextRole).toString(),
                 QStringLiteral("12.00 km"));
        app.sortBy(AppController::SortDistance); // toggle: shortest first
        QCOMPARE(app.runs()->data(app.runs()->index(0), RunListModel::DistanceTextRole).toString(),
                 QStringLiteral("3.00 km"));
        app.sortBy(AppController::SortPace); // fastest first
        QCOMPARE(app.runs()->data(app.runs()->index(0), RunListModel::PaceTextRole).toString(),
                 QStringLiteral("5:00/km"));
        app.sortBy(AppController::SortDate);
        QCOMPARE(app.runs()->data(app.runs()->index(0), RunListModel::DateIsoRole).toString(),
                 QStringLiteral("2026-09-29"));
    }

    void csvExportImport()
    {
        QTemporaryDir home;
        AppController app(home.filePath("plainrun.db"));
        app.setFixedToday(Today);
        QVERIFY(app.initialize());
        QVERIFY(app.saveRun(-1, "2026-09-20", "5", "29:30", "Easy, \"windy\"").value("ok").toBool());
        QVERIFY(app.saveRun(-1, "2026-09-27", "10", "58:00", "Ünïcödé\nsecond line").value("ok").toBool());

        const QUrl file = QUrl::fromLocalFile(home.filePath("runs.csv"));
        QVERIFY(app.exportCsv(file).value("ok").toBool());

        // Importing the same file again adds nothing.
        QVariantMap again = app.importCsv(file);
        QVERIFY(again.value("ok").toBool());
        QCOMPARE(again.value("imported").toInt(), 0);
        QCOMPARE(again.value("skipped").toInt(), 2);
        QCOMPARE(app.totalRunCount(), 2);

        // Into a fresh database, everything arrives intact.
        AppController other(home.filePath("other/plainrun.db"));
        other.setFixedToday(Today);
        QVERIFY(other.initialize());
        QVariantMap imported = other.importCsv(file);
        QVERIFY(imported.value("ok").toBool());
        QCOMPARE(imported.value("imported").toInt(), 2);
        QCOMPARE(state(other), state(app));

        // A file with a bad row imports nothing and reports the line.
        QFile bad(home.filePath("bad.csv"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("date,distance_km,duration\n2026-09-01,5,25:00\n2026-09-02,abc,25:00\n");
        bad.close();
        QVariantMap rejected = other.importCsv(QUrl::fromLocalFile(bad.fileName()));
        QVERIFY(!rejected.value("ok").toBool());
        QVERIFY(rejected.value("details").toString().contains("Line 3"));
        QCOMPARE(other.totalRunCount(), 2);
    }

    void unopenableDatabaseReportsErrorWithoutDestroyingIt()
    {
        QTemporaryDir home;
        const QString path = home.filePath("plainrun.db");
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(4096, 'x'));
        f.close();

        AppController app(path);
        QVERIFY(!app.initialize());
        QVERIFY(!app.ready());
        QVERIFY(!app.startupError().isEmpty());
        QCOMPARE(QFileInfo(path).size(), 4096);
    }
};

QTEST_GUILESS_MAIN(TestController)
#include "tst_controller.moc"
