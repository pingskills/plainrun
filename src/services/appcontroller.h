#pragma once

#include "core/periods.h"
#include "core/run.h"
#include "database/database.h"
#include "models/runlistmodel.h"

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

namespace plainrun {

// The application's single interface for the UI: owns the database connection,
// keeps the in-memory list of runs and exposes derived values to QML.
// Uses only Qt Core/SQL so it can be exercised directly by tests.
class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY stateChanged)
    Q_PROPERTY(QString startupError READ startupError NOTIFY stateChanged)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)
    Q_PROPERTY(QString dataDirectory READ dataDirectory CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(plainrun::RunListModel *runs READ runs CONSTANT)
    Q_PROPERTY(int totalRunCount READ totalRunCount NOTIFY dataChanged)
    Q_PROPERTY(int period READ period WRITE setPeriod NOTIFY viewChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY viewChanged)
    Q_PROPERTY(int sortColumn READ sortColumn NOTIFY viewChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending NOTIFY viewChanged)
    Q_PROPERTY(QVariantMap overview READ overview NOTIFY dataChanged)
    Q_PROPERTY(QVariantMap periodStats READ periodStats NOTIFY viewChanged)
    Q_PROPERTY(QVariantList weeklyChart READ weeklyChart NOTIFY dataChanged)
    Q_PROPERTY(QVariantList monthlyChart READ monthlyChart NOTIFY dataChanged)
    Q_PROPERTY(QVariantList personalBests READ personalBests NOTIFY dataChanged)

public:
    enum SortColumn { SortDate = 0, SortDistance = 1, SortTime = 2, SortPace = 3 };
    Q_ENUM(SortColumn)

    explicit AppController(const QString &databasePath, QObject *parent = nullptr);
    ~AppController() override;

    // Creates the data directory and opens/migrates the database. On failure,
    // ready stays false and startupError explains why. Never resets data.
    bool initialize();

    bool ready() const { return m_ready; }
    QString startupError() const { return m_startupError; }
    QString databasePath() const { return m_databasePath; }
    QString dataDirectory() const;
    QString version() const;
    RunListModel *runs() { return &m_model; }
    int totalRunCount() const { return static_cast<int>(m_allRuns.size()); }

    int period() const { return static_cast<int>(m_period); }
    void setPeriod(int period);
    QString search() const { return m_search; }
    void setSearch(const QString &search);
    int sortColumn() const { return m_sortColumn; }
    bool sortAscending() const { return m_sortAscending; }

    QVariantMap overview() const { return m_overview; }
    QVariantMap periodStats() const { return m_periodStats; }
    QVariantList weeklyChart() const { return m_weeklyChart; }
    QVariantList monthlyChart() const { return m_monthlyChart; }
    QVariantList personalBests() const { return m_personalBests; }

    // Clicking a column header: same column toggles direction; a new column
    // starts in its natural direction (newest date, longest, longest, fastest).
    Q_INVOKABLE void sortBy(int column);

    // Live validation for the run form. Returns {ok, dateError, distanceError,
    // durationError, paceText, dateLong}.
    Q_INVOKABLE QVariantMap validateRun(const QString &date, const QString &distance,
                                        const QString &duration) const;
    // Adds (id <= 0) or updates a run. Returns {ok, id, error, ...field errors}.
    Q_INVOKABLE QVariantMap saveRun(qint64 id, const QString &date, const QString &distance,
                                    const QString &duration, const QString &note);
    Q_INVOKABLE bool deleteRun(qint64 id);
    // {found, id, dateIso, dateLong, distanceText, distanceInput, durationText,
    //  paceText, note}
    Q_INVOKABLE QVariantMap runDetails(qint64 id) const;

    Q_INVOKABLE QString todayIso() const;
    Q_INVOKABLE QString shiftDate(const QString &iso, int days) const;
    Q_INVOKABLE QString longDate(const QString &iso) const;

    Q_INVOKABLE QString defaultBackupFileName() const;
    Q_INVOKABLE QString defaultExportFileName() const;
    Q_INVOKABLE QVariantMap backupTo(const QUrl &file);
    // {valid, error, runCount, latestRun, currentRunCount}
    Q_INVOKABLE QVariantMap inspectBackup(const QUrl &file) const;
    Q_INVOKABLE QVariantMap restoreFrom(const QUrl &file);
    Q_INVOKABLE QVariantMap exportCsv(const QUrl &file);
    Q_INVOKABLE QVariantMap importCsv(const QUrl &file);

    // Testing hook: pin "today" so date-relative views are deterministic.
    void setFixedToday(const QDate &date);
    QDate today() const;
    const QList<Run> &allRuns() const { return m_allRuns; }

signals:
    void stateChanged();
    void dataChanged();
    void viewChanged();

private:
    bool reload();
    void recomputeData();
    void recomputeView();
    void checkDateRollover();
    static QString toLocalPath(const QUrl &url);

    QString m_databasePath;
    db::Database m_db;
    bool m_ready = false;
    QString m_startupError;

    QList<Run> m_allRuns;
    RunListModel m_model;
    Period m_period = Period::All;
    QString m_search;
    int m_sortColumn = SortDate;
    bool m_sortAscending = false;

    QVariantMap m_overview;
    QVariantMap m_periodStats;
    QVariantList m_weeklyChart;
    QVariantList m_monthlyChart;
    QVariantList m_personalBests;

    QDate m_fixedToday;
    QDate m_lastSeenToday;
    QTimer m_dayTimer;
};

} // namespace plainrun
