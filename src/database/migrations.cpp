#include "database/migrations.h"

#include "core/logging.h"
#include "database/database.h"

#include <QSqlError>
#include <QSqlQuery>

namespace plainrun::db {

const QList<Migration> &migrations()
{
    static const QList<Migration> list = {
        {1,
         {
             QStringLiteral("PRAGMA application_id = %1").arg(Database::ApplicationId),
             QStringLiteral(
                 "CREATE TABLE runs ("
                 "  id INTEGER PRIMARY KEY,"
                 "  run_date TEXT NOT NULL"
                 "    CHECK (run_date GLOB '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'),"
                 "  distance_metres INTEGER NOT NULL CHECK (distance_metres > 0),"
                 "  duration_seconds INTEGER NOT NULL CHECK (duration_seconds > 0),"
                 "  note TEXT NOT NULL DEFAULT '',"
                 "  created_at TEXT NOT NULL,"
                 "  updated_at TEXT NOT NULL"
                 ")"),
             QStringLiteral("CREATE INDEX runs_run_date_idx ON runs (run_date)"),
         }},
        {2,
         {
             // Optional average heart rate; NULL when not recorded.
             QStringLiteral(
                 "ALTER TABLE runs ADD COLUMN avg_heart_rate_bpm INTEGER"
                 "  CHECK (avg_heart_rate_bpm IS NULL OR avg_heart_rate_bpm BETWEEN 30 AND 250)"),
         }},
    };
    return list;
}

int latestSchemaVersion()
{
    return migrations().isEmpty() ? 0 : migrations().back().version;
}

int userVersion(QSqlDatabase &db)
{
    QSqlQuery q(db);
    if (q.exec(QStringLiteral("PRAGMA user_version")) && q.next())
        return q.value(0).toInt();
    return -1;
}

bool migrate(QSqlDatabase &db, const QList<Migration> &steps, QString *error)
{
    const int current = userVersion(db);
    if (current < 0) {
        if (error)
            *error = QStringLiteral("could not read schema version: %1").arg(db.lastError().text());
        return false;
    }

    for (const Migration &m : steps) {
        if (m.version <= current)
            continue;

        qCInfo(lcDb) << "applying migration to schema version" << m.version;
        if (!db.transaction()) {
            if (error)
                *error = QStringLiteral("could not begin transaction: %1").arg(db.lastError().text());
            return false;
        }

        QString failure;
        QSqlQuery q(db);
        for (const QString &sql : m.statements) {
            if (!q.exec(sql)) {
                failure = QStringLiteral("migration %1 failed: %2").arg(m.version).arg(q.lastError().text());
                break;
            }
        }
        if (failure.isEmpty() && !q.exec(QStringLiteral("PRAGMA user_version = %1").arg(m.version)))
            failure = QStringLiteral("could not record schema version %1: %2").arg(m.version).arg(q.lastError().text());
        q.finish();

        if (!failure.isEmpty()) {
            db.rollback();
            qCWarning(lcDb).noquote() << failure;
            if (error)
                *error = failure;
            return false;
        }
        if (!db.commit()) {
            const QString msg = QStringLiteral("could not commit migration %1: %2").arg(m.version).arg(db.lastError().text());
            db.rollback();
            qCWarning(lcDb).noquote() << msg;
            if (error)
                *error = msg;
            return false;
        }
    }
    return true;
}

} // namespace plainrun::db
