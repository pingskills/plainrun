// Development only (built with -DPLAINRUN_BUILD_DEVTOOLS=ON, never installed).
// Fills a sandbox database with about a year of synthetic runs so the UI can be
// exercised with realistic data. Refuses to touch the real per-user database.
//
//   plainrun-devseed /tmp/plainrun-sandbox/plainrun/plainrun.db
//   XDG_DATA_HOME=/tmp/plainrun-sandbox plainrun
#include "database/database.h"
#include "database/runrepository.h"
#include "services/datalocation.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QTextStream>

using namespace plainrun;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plainrun"));
    QTextStream err(stderr);

    const QStringList args = app.arguments();
    if (args.size() != 2) {
        err << "usage: plainrun-devseed <sandbox-database-path>\n";
        return 2;
    }
    const QString target = QFileInfo(args.at(1)).absoluteFilePath();
    const QString real = QFileInfo(QDir::home().filePath(QStringLiteral(".local/share/plainrun/plainrun.db")))
                             .absoluteFilePath();
    if (target == real || target == QFileInfo(databasePath()).absoluteFilePath()) {
        err << "refusing to seed the real per-user database: " << target << "\n";
        return 2;
    }
    if (QFileInfo::exists(target)) {
        err << "refusing to seed an existing file: " << target << "\n";
        return 2;
    }
    QDir().mkpath(QFileInfo(target).absolutePath());

    db::Database database;
    if (!database.open(target)) {
        err << database.lastError() << "\n";
        return 1;
    }

    QRandomGenerator rng(20260930); // deterministic
    const QStringList notes = {
        QString(), QString(), QString(), QStringLiteral("Easy run around Karkarook"),
        QStringLiteral("parkrun"), QStringLiteral("Hills, windy"), QStringLiteral("Recovery jog"),
        QStringLiteral("Long run along the bay — felt strong"),
    };
    QList<Run> runs;
    const QDate today = QDate::currentDate();
    for (QDate d = today.addDays(-400); d <= today; d = d.addDays(1)) {
        const int dow = d.dayOfWeek();
        const bool runDay = dow == 2 || dow == 4 || dow == 6 || (dow == 7 && rng.bounded(3) == 0);
        if (!runDay || rng.bounded(6) == 0)
            continue;
        Run r;
        r.date = d;
        if (dow == 6) {
            r.distanceMetres = 5000 + rng.bounded(40);
        } else if (dow == 7) {
            r.distanceMetres = 12000 + rng.bounded(9000);
        } else {
            r.distanceMetres = 4000 + rng.bounded(5000);
        }
        const int paceSeconds = (dow == 6 ? 300 : 335) + rng.bounded(40) - 20;
        r.durationSeconds = r.distanceMetres * paceSeconds / 1000;
        r.note = notes.at(rng.bounded(notes.size()));
        // Heart rate from a watch bought ~10 months ago, sometimes forgotten.
        // Fitness improves: the same effort costs a few beats less over time.
        const qint64 daysAgo = d.daysTo(today);
        if (daysAgo < 300 && rng.bounded(8) != 0)
            r.heartRateBpm = 141 + int(daysAgo / 40) + (dow == 6 ? 12 : 0) + rng.bounded(9) - 4;
        runs.append(r);
    }
    // A few race-distance runs so personal bests appear.
    for (const auto &[days, metres, seconds] : {std::tuple{120, 10020, 2950}, std::tuple{60, 21130, 6480},
                                                std::tuple{200, 1000, 228}}) {
        Run r;
        r.date = today.addDays(-days);
        r.distanceMetres = metres;
        r.durationSeconds = seconds;
        r.note = QStringLiteral("Race");
        runs.append(r);
    }

    QString error;
    if (!db::insertRuns(database.connection(), runs, &error)) {
        err << error << "\n";
        return 1;
    }
    QTextStream(stdout) << "Seeded " << runs.size() << " runs into " << target << "\n";
    return 0;
}
