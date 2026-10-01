#include "database/runrepository.h"

#include "core/logging.h"
#include "database/migrations.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace plainrun::db {

namespace {

QString timestampNow()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

Run runFromQuery(const QSqlQuery &q)
{
    Run r;
    r.id = q.value(0).toLongLong();
    r.date = QDate::fromString(q.value(1).toString(), Qt::ISODate);
    r.distanceMetres = q.value(2).toLongLong();
    r.durationSeconds = q.value(3).toLongLong();
    r.note = q.value(4).toString();
    r.createdAt = QDateTime::fromString(q.value(5).toString(), Qt::ISODate);
    r.updatedAt = QDateTime::fromString(q.value(6).toString(), Qt::ISODate);
    r.heartRateBpm = q.value(7).isNull() ? 0 : q.value(7).toInt();
    return r;
}

// The note column is NOT NULL; a null QString would bind as SQL NULL.
QString noteValue(const Run &run)
{
    return run.note.isNull() ? QStringLiteral("") : run.note;
}

// Heart rate 0 means "not recorded" and is stored as NULL.
QVariant heartRateValue(const Run &run)
{
    return run.heartRateBpm > 0 ? QVariant(run.heartRateBpm) : QVariant(QMetaType::fromType<int>());
}

// A read-only connection (the CLI) is never migrated, so it may be reading an
// older schema: select NULL for columns that schema doesn't have yet.
QString selectColumns(QSqlDatabase &db)
{
    const QString heartRate = userVersion(db) >= 2 ? QStringLiteral("avg_heart_rate_bpm")
                                                   : QStringLiteral("NULL");
    return QStringLiteral("SELECT id, run_date, distance_metres, duration_seconds, note, created_at, updated_at, "
                          "%1 FROM runs").arg(heartRate);
}

bool report(const QSqlQuery &q, QString *error)
{
    qCWarning(lcDb).noquote() << "query failed:" << q.lastError().text();
    if (error)
        *error = q.lastError().text();
    return false;
}

bool insertOne(QSqlDatabase &db, Run &run, QString *error)
{
    const QString now = timestampNow();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO runs (run_date, distance_metres, duration_seconds, note, avg_heart_rate_bpm, "
        "created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?)"));
    q.addBindValue(run.date.toString(Qt::ISODate));
    q.addBindValue(run.distanceMetres);
    q.addBindValue(run.durationSeconds);
    q.addBindValue(noteValue(run));
    q.addBindValue(heartRateValue(run));
    q.addBindValue(now);
    q.addBindValue(now);
    if (!q.exec())
        return report(q, error);
    run.id = q.lastInsertId().toLongLong();
    run.createdAt = run.updatedAt = QDateTime::fromString(now, Qt::ISODate);
    return true;
}

} // namespace

QList<Run> allRuns(QSqlDatabase db, QString *error)
{
    QList<Run> runs;
    QSqlQuery q(db);
    q.setForwardOnly(true);
    if (!q.exec(selectColumns(db) + QStringLiteral(" ORDER BY run_date DESC, id DESC"))) {
        report(q, error);
        return runs;
    }
    while (q.next())
        runs.append(runFromQuery(q));
    return runs;
}

std::optional<Run> findRun(QSqlDatabase db, qint64 id)
{
    QSqlQuery q(db);
    q.prepare(selectColumns(db) + QStringLiteral(" WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return runFromQuery(q);
}

bool insertRun(QSqlDatabase db, Run &run, QString *error)
{
    return insertOne(db, run, error);
}

bool updateRun(QSqlDatabase db, Run &run, QString *error)
{
    const QString now = timestampNow();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE runs SET run_date = ?, distance_metres = ?, duration_seconds = ?, note = ?, "
        "avg_heart_rate_bpm = ?, updated_at = ? WHERE id = ?"));
    q.addBindValue(run.date.toString(Qt::ISODate));
    q.addBindValue(run.distanceMetres);
    q.addBindValue(run.durationSeconds);
    q.addBindValue(noteValue(run));
    q.addBindValue(heartRateValue(run));
    q.addBindValue(now);
    q.addBindValue(run.id);
    if (!q.exec())
        return report(q, error);
    if (q.numRowsAffected() != 1) {
        if (error)
            *error = QStringLiteral("run %1 no longer exists").arg(run.id);
        return false;
    }
    run.updatedAt = QDateTime::fromString(now, Qt::ISODate);
    return true;
}

bool deleteRun(QSqlDatabase db, qint64 id, QString *error)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM runs WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec())
        return report(q, error);
    return true;
}

bool insertRuns(QSqlDatabase db, QList<Run> &runs, QString *error)
{
    if (!db.transaction()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }
    for (Run &run : runs) {
        if (!insertOne(db, run, error)) {
            db.rollback();
            for (Run &r : runs)
                r.id = 0;
            return false;
        }
    }
    if (!db.commit()) {
        if (error)
            *error = db.lastError().text();
        db.rollback();
        return false;
    }
    return true;
}

} // namespace plainrun::db
