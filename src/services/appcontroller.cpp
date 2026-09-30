#include "services/appcontroller.h"

#include "core/csv.h"
#include "core/logging.h"
#include "core/personalbests.h"
#include "core/runformat.h"
#include "core/statistics.h"
#include "core/validation.h"
#include "database/backup.h"
#include "database/runrepository.h"
#include "plainrun_config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>

#include <algorithm>

namespace plainrun {

namespace {

constexpr int WeeklyChartWeeks = 12;
constexpr int MonthlyChartMonths = 12;
constexpr qint64 MaxImportBytes = 50 * 1024 * 1024;
constexpr int MaxReportedProblems = 8;

QString kmText(qint64 metres, int decimals)
{
    return formatKm(metres, decimals, QLocale()) + QStringLiteral(" km");
}

QString paceText(qint64 metres, qint64 seconds)
{
    const qint64 pace = paceSecondsPerKm(metres, seconds);
    return pace > 0 ? formatPace(pace) + QStringLiteral("/km") : QStringLiteral("–");
}

QString daysAgoText(const QDate &date, const QDate &today)
{
    const qint64 days = date.daysTo(today);
    if (days <= 0)
        return AppController::tr("today");
    if (days == 1)
        return AppController::tr("yesterday");
    return AppController::tr("%1 days ago").arg(days);
}

// English singular/plural without requiring a translation catalogue.
QString count(qint64 n, const QString &singular, const QString &plural)
{
    return QStringLiteral("%1 %2").arg(n).arg(n == 1 ? singular : plural);
}

QString runsText(qint64 n)
{
    return count(n, AppController::tr("run"), AppController::tr("runs"));
}

QVariantMap result(bool ok, const QString &message = {})
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("message"), message}};
}

} // namespace

AppController::AppController(const QString &databasePath, QObject *parent)
    : QObject(parent)
    , m_databasePath(databasePath)
{
    // Keep "this week" and friends correct if PlainRun stays open past midnight.
    m_dayTimer.setInterval(60 * 1000);
    connect(&m_dayTimer, &QTimer::timeout, this, &AppController::checkDateRollover);
}

AppController::~AppController() = default;

bool AppController::initialize()
{
    const QString dir = QFileInfo(m_databasePath).absolutePath();
    if (!QDir().mkpath(dir)) {
        m_startupError = tr("Could not create the data folder %1.").arg(dir);
        qCWarning(lcApp).noquote() << m_startupError;
        emit stateChanged();
        return false;
    }
    if (!m_db.open(m_databasePath)) {
        m_startupError = m_db.lastError();
        emit stateChanged();
        return false;
    }
    if (!reload()) {
        emit stateChanged();
        return false;
    }
    m_ready = true;
    m_startupError.clear();
    m_lastSeenToday = today();
    m_dayTimer.start();
    emit stateChanged();
    return true;
}

QString AppController::dataDirectory() const
{
    return QFileInfo(m_databasePath).absolutePath();
}

QString AppController::version() const
{
    return QStringLiteral(PLAINRUN_VERSION);
}

void AppController::setFixedToday(const QDate &date)
{
    m_fixedToday = date;
    recomputeData();
}

QDate AppController::today() const
{
    return m_fixedToday.isValid() ? m_fixedToday : QDate::currentDate();
}

void AppController::checkDateRollover()
{
    if (today() != m_lastSeenToday) {
        m_lastSeenToday = today();
        recomputeData();
    }
}

bool AppController::reload()
{
    QString error;
    QList<Run> runs = db::allRuns(m_db.connection(), &error);
    if (!error.isEmpty()) {
        m_startupError = tr("Could not read your runs: %1").arg(error);
        return false;
    }
    m_allRuns = std::move(runs);
    recomputeData();
    return true;
}

void AppController::recomputeData()
{
    const QDate now = today();
    const QLocale locale;
    const Overview o = overviewFor(m_allRuns, now);

    QVariantMap ov;
    ov[QStringLiteral("hasRuns")] = o.lastRun.has_value();
    if (o.lastRun) {
        const Run &r = *o.lastRun;
        ov[QStringLiteral("lastRunId")] = r.id;
        ov[QStringLiteral("lastRunDate")] = RunListModel::shortDate(r.date, now, locale);
        ov[QStringLiteral("lastRunAgo")] = daysAgoText(r.date, now);
        ov[QStringLiteral("lastRunDistance")] = kmText(r.distanceMetres, 2);
        ov[QStringLiteral("lastRunDuration")] = formatDuration(r.durationSeconds);
        ov[QStringLiteral("lastRunPace")] = paceText(r.distanceMetres, r.durationSeconds);
    }
    auto addTotals = [&](const QString &prefix, const Totals &t) {
        ov[prefix + QStringLiteral("Runs")] = t.runs;
        ov[prefix + QStringLiteral("Km")] = kmText(t.metres, 1);
        ov[prefix + QStringLiteral("Metres")] = t.metres;
        ov[prefix + QStringLiteral("Time")] = formatDurationShort(t.seconds);
        ov[prefix + QStringLiteral("Pace")] = paceText(t.metres, t.seconds);
    };
    addTotals(QStringLiteral("week"), o.thisWeek);
    addTotals(QStringLiteral("prevWeek"), o.previousWeek);
    addTotals(QStringLiteral("month"), o.thisMonth);
    addTotals(QStringLiteral("prevMonth"), o.previousMonth);
    addTotals(QStringLiteral("year"), o.thisYear);
    m_overview = ov;

    QVariantList weekly;
    const QList<Bucket> weeks = weeklyBuckets(m_allRuns, now, WeeklyChartWeeks);
    for (qsizetype i = 0; i < weeks.size(); ++i) {
        const Bucket &b = weeks.at(i);
        weekly.append(QVariantMap{
            {QStringLiteral("label"), locale.toString(b.range.first, QStringLiteral("d MMM"))},
            {QStringLiteral("value"), static_cast<double>(b.totals.metres) / 1000.0},
            {QStringLiteral("valueText"), kmText(b.totals.metres, 1)},
            {QStringLiteral("detail"), tr("Week of %1: %2, %3")
                                          .arg(locale.toString(b.range.first, QStringLiteral("d MMM yyyy")),
                                               kmText(b.totals.metres, 1), runsText(b.totals.runs))},
            {QStringLiteral("current"), i == weeks.size() - 1},
        });
    }
    m_weeklyChart = weekly;

    QVariantList monthly;
    const QList<Bucket> months = monthlyBuckets(m_allRuns, now, MonthlyChartMonths);
    for (qsizetype i = 0; i < months.size(); ++i) {
        const Bucket &b = months.at(i);
        monthly.append(QVariantMap{
            {QStringLiteral("label"), locale.toString(b.range.first, QStringLiteral("MMM"))},
            {QStringLiteral("value"), static_cast<double>(b.totals.metres) / 1000.0},
            {QStringLiteral("valueText"), kmText(b.totals.metres, 1)},
            {QStringLiteral("detail"), tr("%1: %2, %3")
                                          .arg(locale.toString(b.range.first, QStringLiteral("MMMM yyyy")),
                                               kmText(b.totals.metres, 1), runsText(b.totals.runs))},
            {QStringLiteral("current"), i == months.size() - 1},
        });
    }
    m_monthlyChart = monthly;

    QVariantList pbs;
    for (const PersonalBest &pb : plainrun::personalBests(m_allRuns)) {
        pbs.append(QVariantMap{
            {QStringLiteral("label"), pb.target.label},
            {QStringLiteral("timeText"), formatDuration(pb.run.durationSeconds)},
            {QStringLiteral("dateText"), locale.toString(pb.run.date, QStringLiteral("d MMM yyyy"))},
            {QStringLiteral("distanceText"), kmText(pb.run.distanceMetres, 2)},
            {QStringLiteral("paceText"), paceText(pb.run.distanceMetres, pb.run.durationSeconds)},
            {QStringLiteral("runId"), pb.run.id},
        });
    }
    m_personalBests = pbs;

    emit dataChanged();
    recomputeView();
}

void AppController::recomputeView()
{
    const QDate now = today();
    const DateRange range = rangeFor(m_period, now);
    const QString needle = m_search.trimmed();

    QList<Run> shown;
    for (const Run &r : std::as_const(m_allRuns)) {
        if (!range.contains(r.date))
            continue;
        if (!needle.isEmpty() && !r.note.contains(needle, Qt::CaseInsensitive))
            continue;
        shown.append(r);
    }

    auto key = [this](const Run &a, const Run &b) -> int {
        auto cmp = [](qint64 x, qint64 y) { return x < y ? -1 : (x > y ? 1 : 0); };
        switch (m_sortColumn) {
        case SortDistance:
            return cmp(a.distanceMetres, b.distanceMetres);
        case SortTime:
            return cmp(a.durationSeconds, b.durationSeconds);
        case SortPace:
            return cmp(paceSecondsPerKm(a.distanceMetres, a.durationSeconds),
                       paceSecondsPerKm(b.distanceMetres, b.durationSeconds));
        default:
            return 0;
        }
    };
    std::stable_sort(shown.begin(), shown.end(), [&](const Run &a, const Run &b) {
        const int c = key(a, b);
        if (c != 0)
            return m_sortAscending ? c < 0 : c > 0;
        // Date order (and the tie-break for other columns).
        if (m_sortColumn == SortDate && m_sortAscending)
            return newerThan(b, a);
        return newerThan(a, b);
    });

    m_model.setRuns(shown, now);

    const Totals t = totalsIn(shown, {});
    static const QStringList labels = {tr("This week"), tr("This month"), tr("This year"), tr("All runs")};
    QVariantMap stats;
    stats[QStringLiteral("label")] = labels.value(static_cast<int>(m_period));
    stats[QStringLiteral("runs")] = t.runs;
    stats[QStringLiteral("distanceText")] = kmText(t.metres, 1);
    stats[QStringLiteral("timeText")] = formatDurationShort(t.seconds);
    stats[QStringLiteral("timeExact")] = formatDuration(t.seconds);
    stats[QStringLiteral("averageDistanceText")] = kmText(t.averageMetres(), 2);
    stats[QStringLiteral("paceText")] = paceText(t.metres, t.seconds);
    stats[QStringLiteral("filtered")] = !needle.isEmpty();
    m_periodStats = stats;

    emit viewChanged();
}

void AppController::setPeriod(int period)
{
    const Period p = static_cast<Period>(std::clamp(period, 0, 3));
    if (p == m_period)
        return;
    m_period = p;
    recomputeView();
}

void AppController::setSearch(const QString &search)
{
    if (search == m_search)
        return;
    m_search = search;
    recomputeView();
}

void AppController::sortBy(int column)
{
    column = std::clamp(column, 0, 3);
    if (column == m_sortColumn) {
        m_sortAscending = !m_sortAscending;
    } else {
        m_sortColumn = column;
        m_sortAscending = column == SortPace; // fastest pace first; otherwise largest/newest first
    }
    recomputeView();
}

QVariantMap AppController::validateRun(const QString &date, const QString &distance,
                                       const QString &duration) const
{
    const RunValidation v = validateRunInput(date, distance, duration, today());
    QVariantMap m;
    m[QStringLiteral("ok")] = v.ok();
    m[QStringLiteral("dateError")] = v.dateError;
    m[QStringLiteral("distanceError")] = v.distanceError;
    m[QStringLiteral("durationError")] = v.durationError;
    const bool paceKnown = v.distanceError.isEmpty() && v.durationError.isEmpty();
    m[QStringLiteral("paceText")] = paceKnown ? paceText(v.distanceMetres, v.durationSeconds) : QString();
    m[QStringLiteral("dateLong")] = v.date.isValid() ? QLocale().toString(v.date, QStringLiteral("dddd d MMMM yyyy"))
                                                     : QString();
    return m;
}

QVariantMap AppController::saveRun(qint64 id, const QString &date, const QString &distance,
                                   const QString &duration, const QString &note)
{
    QVariantMap out = validateRun(date, distance, duration);
    if (!out.value(QStringLiteral("ok")).toBool()) {
        out[QStringLiteral("message")] = tr("Please check the highlighted fields.");
        return out;
    }
    if (!m_db.isOpen()) {
        out[QStringLiteral("ok")] = false;
        out[QStringLiteral("message")] = tr("The database is not open.");
        return out;
    }

    const RunValidation v = validateRunInput(date, distance, duration, today());
    Run run;
    run.id = id > 0 ? id : 0;
    run.date = v.date;
    run.distanceMetres = v.distanceMetres;
    run.durationSeconds = v.durationSeconds;
    run.note = normaliseNote(note);

    QString error;
    const bool ok = run.id > 0 ? db::updateRun(m_db.connection(), run, &error)
                               : db::insertRun(m_db.connection(), run, &error);
    if (!ok) {
        out[QStringLiteral("ok")] = false;
        out[QStringLiteral("message")] = tr("The run could not be saved: %1").arg(error);
        return out;
    }
    reload();
    out[QStringLiteral("id")] = run.id;
    return out;
}

bool AppController::deleteRun(qint64 id)
{
    QString error;
    if (!db::deleteRun(m_db.connection(), id, &error))
        return false;
    reload();
    return true;
}

QVariantMap AppController::runDetails(qint64 id) const
{
    const auto it = std::find_if(m_allRuns.cbegin(), m_allRuns.cend(), [id](const Run &r) { return r.id == id; });
    if (it == m_allRuns.cend())
        return {{QStringLiteral("found"), false}};
    const Run &r = *it;
    const QLocale locale;
    return {
        {QStringLiteral("found"), true},
        {QStringLiteral("id"), r.id},
        {QStringLiteral("dateIso"), formatIsoDate(r.date)},
        {QStringLiteral("dateLong"), locale.toString(r.date, QStringLiteral("dddd d MMMM yyyy"))},
        {QStringLiteral("distanceText"), kmText(r.distanceMetres, 2)},
        // Editable form value: C-locale decimals, trailing zeros trimmed ("5", "7.2").
        {QStringLiteral("distanceInput"), QString::number(static_cast<double>(r.distanceMetres) / 1000.0, 'f', 3)
                                               .remove(QRegularExpression(QStringLiteral("\\.?0+$")))},
        {QStringLiteral("durationText"), formatDuration(r.durationSeconds)},
        {QStringLiteral("paceText"), paceText(r.distanceMetres, r.durationSeconds)},
        {QStringLiteral("note"), r.note},
    };
}

QString AppController::todayIso() const
{
    return formatIsoDate(today());
}

QString AppController::shiftDate(const QString &iso, int days) const
{
    const auto d = parseIsoDate(iso);
    const QDate base = d ? *d : today();
    return formatIsoDate(base.addDays(days));
}

QString AppController::longDate(const QString &iso) const
{
    const auto d = parseIsoDate(iso);
    return d ? QLocale().toString(*d, QStringLiteral("dddd d MMMM yyyy")) : QString();
}

QString AppController::defaultBackupFileName() const
{
    return QStringLiteral("plainrun-backup-%1.db").arg(todayIso());
}

QString AppController::defaultExportFileName() const
{
    return QStringLiteral("plainrun-runs-%1.csv").arg(todayIso());
}

QString AppController::toLocalPath(const QUrl &url)
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString(QUrl::PreferLocalFile);
}

QVariantMap AppController::backupTo(const QUrl &file)
{
    const QString path = toLocalPath(file);
    if (QFileInfo(path).absoluteFilePath() == QFileInfo(m_databasePath).absoluteFilePath())
        return result(false, tr("Choose a different file: that is PlainRun's live database."));
    QString error;
    if (!db::writeBackup(m_db, path, &error))
        return result(false, error);
    return result(true, tr("Backed up %1 to %2.").arg(runsText(totalRunCount()), path));
}

QVariantMap AppController::inspectBackup(const QUrl &file) const
{
    const db::BackupInfo info = db::inspectBackup(toLocalPath(file));
    return {
        {QStringLiteral("valid"), info.valid},
        {QStringLiteral("error"), info.error},
        {QStringLiteral("runCount"), info.runCount},
        {QStringLiteral("latestRun"), info.latestRun.isValid()
                                          ? QLocale().toString(info.latestRun, QStringLiteral("d MMM yyyy"))
                                          : QString()},
        {QStringLiteral("currentRunCount"), totalRunCount()},
    };
}

QVariantMap AppController::restoreFrom(const QUrl &file)
{
    QString safety;
    QString error;
    const bool ok = db::restoreBackup(m_db, toLocalPath(file), &safety, &error);
    if (!m_db.isOpen()) {
        m_ready = false;
        m_startupError = tr("The database could not be reopened after restoring: %1").arg(error);
        emit stateChanged();
        return result(false, m_startupError);
    }
    reload();
    if (!ok)
        return result(false, error);
    QVariantMap out = result(true, tr("Restored %1. Your previous data was saved as %2.")
                                          .arg(runsText(totalRunCount()), QFileInfo(safety).fileName()));
    out[QStringLiteral("safetyCopy")] = safety;
    return out;
}

QVariantMap AppController::exportCsv(const QUrl &file)
{
    const QString path = toLocalPath(file);
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly))
        return result(false, tr("Could not write %1: %2").arg(path, out.errorString()));
    out.write(csv::exportRuns(m_allRuns).toUtf8());
    if (!out.commit())
        return result(false, tr("Could not write %1: %2").arg(path, out.errorString()));
    return result(true, tr("Exported %1 to %2.").arg(runsText(totalRunCount()), path));
}

QVariantMap AppController::importCsv(const QUrl &file)
{
    const QString path = toLocalPath(file);
    QFile in(path);
    if (!in.open(QIODevice::ReadOnly))
        return result(false, tr("Could not read %1: %2").arg(path, in.errorString()));
    if (in.size() > MaxImportBytes)
        return result(false, tr("The file is too large to be a PlainRun CSV export."));

    const csv::ImportResult parsed = csv::parseRuns(QString::fromUtf8(in.readAll()), today());
    if (!parsed.problems.isEmpty()) {
        QStringList shown = parsed.problems.mid(0, MaxReportedProblems);
        if (parsed.problems.size() > MaxReportedProblems)
            shown << tr("…and %1 more.").arg(parsed.problems.size() - MaxReportedProblems);
        QVariantMap out = result(false, tr("Nothing was imported. Please fix these rows and try again:"));
        out[QStringLiteral("details")] = shown.join(QLatin1Char('\n'));
        return out;
    }

    QSet<QString> existing;
    for (const Run &r : std::as_const(m_allRuns))
        existing.insert(duplicateKey(r));
    QList<Run> fresh;
    int duplicates = 0;
    for (const csv::ImportRow &row : parsed.rows) {
        if (existing.contains(duplicateKey(row.run)))
            ++duplicates;
        else
            fresh.append(row.run);
    }

    if (!fresh.isEmpty()) {
        QString error;
        if (!db::insertRuns(m_db.connection(), fresh, &error))
            return result(false, tr("Nothing was imported: %1").arg(error));
        reload();
    }

    QString message = tr("Imported %1.").arg(runsText(fresh.size()));
    if (duplicates > 0)
        message += QLatin1Char(' ') + tr("Skipped %1 already recorded.").arg(duplicates);
    QVariantMap out = result(true, message);
    out[QStringLiteral("imported")] = static_cast<int>(fresh.size());
    out[QStringLiteral("skipped")] = duplicates;
    return out;
}

} // namespace plainrun
