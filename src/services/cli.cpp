#include "services/cli.h"

#include "core/runformat.h"
#include "core/statistics.h"
#include "database/database.h"
#include "database/runrepository.h"
#include "plainrun_config.h"

#include <QCommandLineParser>
#include <QFileInfo>

#include <cstring>

namespace plainrun::cli {

namespace {

const char *const Commands[] = {
    "--week-distance", "--month-distance", "--year-distance", "--last-run",
    "--version", "-v", "--help", "-h", "--help-all",
};

} // namespace

bool isCliInvocation(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        for (const char *cmd : Commands) {
            if (std::strcmp(argv[i], cmd) == 0)
                return true;
        }
    }
    return false;
}

int run(const QStringList &arguments, const QString &databasePath, QTextStream &out,
        QTextStream &err, const QDate &today)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(PLAINRUN_DESCRIPTION));
    const QCommandLineOption help({QStringLiteral("h"), QStringLiteral("help")},
                                  QStringLiteral("Show this help and exit."));
    const QCommandLineOption version({QStringLiteral("v"), QStringLiteral("version")},
                                     QStringLiteral("Show the version and exit."));
    const QCommandLineOption week(QStringLiteral("week-distance"),
                                  QStringLiteral("Kilometres run this calendar week (Monday start)."));
    const QCommandLineOption month(QStringLiteral("month-distance"),
                                   QStringLiteral("Kilometres run this calendar month."));
    const QCommandLineOption year(QStringLiteral("year-distance"),
                                  QStringLiteral("Kilometres run this calendar year."));
    const QCommandLineOption last(QStringLiteral("last-run"),
                                  QStringLiteral("Most recent run as: date<TAB>km<TAB>duration<TAB>pace."));
    parser.addOptions({help, version, week, month, year, last});

    if (!parser.parse(arguments)) {
        err << parser.errorText() << "\n" << "Try 'plainrun --help'.\n";
        return ExitUsage;
    }
    if (!parser.positionalArguments().isEmpty()) {
        err << "Unexpected argument: " << parser.positionalArguments().first() << "\n";
        return ExitUsage;
    }
    if (parser.isSet(help)) {
        out << parser.helpText()
            << "\nWith no options, PlainRun opens its window.\n"
               "Exit codes: 0 success, 1 database error, 2 usage error, 3 no runs recorded.\n";
        return ExitOk;
    }
    if (parser.isSet(version)) {
        out << "plainrun " << PLAINRUN_VERSION << "\n";
        return ExitOk;
    }

    const int chosen = int(parser.isSet(week)) + int(parser.isSet(month)) + int(parser.isSet(year))
        + int(parser.isSet(last));
    if (chosen != 1) {
        err << "Choose exactly one of --week-distance, --month-distance, --year-distance, --last-run.\n";
        return ExitUsage;
    }

    // Never create a database from the CLI: no file simply means no runs yet.
    QList<Run> runs;
    if (QFileInfo::exists(databasePath)) {
        db::Database database;
        if (!database.open(databasePath, db::Database::Mode::ReadOnly)) {
            err << "plainrun: " << database.lastError() << "\n";
            return ExitDatabaseError;
        }
        QString error;
        runs = db::allRuns(database.connection(), &error);
        if (!error.isEmpty()) {
            err << "plainrun: could not read runs: " << error << "\n";
            return ExitDatabaseError;
        }
    }

    const Overview o = overviewFor(runs, today);
    if (parser.isSet(week)) {
        out << formatKm(o.thisWeek.metres, 2) << "\n";
    } else if (parser.isSet(month)) {
        out << formatKm(o.thisMonth.metres, 2) << "\n";
    } else if (parser.isSet(year)) {
        out << formatKm(o.thisYear.metres, 2) << "\n";
    } else {
        if (!o.lastRun)
            return ExitNoRuns;
        const Run &r = *o.lastRun;
        out << formatIsoDate(r.date) << '\t' << formatKm(r.distanceMetres, 2) << '\t'
            << formatDuration(r.durationSeconds) << '\t'
            << formatPace(paceSecondsPerKm(r.distanceMetres, r.durationSeconds)) << "\n";
    }
    return ExitOk;
}

} // namespace plainrun::cli
