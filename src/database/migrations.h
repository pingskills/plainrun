#pragma once

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>

namespace plainrun::db {

// A migration upgrades the schema from version (version - 1) to `version`.
// Migrations are append-only: never edit a released migration, add a new one.
struct Migration {
    int version = 0;
    QStringList statements;
};

// The migrations shipped with this build, in ascending version order.
const QList<Migration> &migrations();

// Highest schema version this build understands.
int latestSchemaVersion();

// Applies every migration whose version is greater than the database's current
// PRAGMA user_version. Each migration runs in its own transaction together with
// the user_version bump, so a failure leaves the database at the previous
// version with its data intact.
bool migrate(QSqlDatabase &db, const QList<Migration> &steps, QString *error);

int userVersion(QSqlDatabase &db);

} // namespace plainrun::db
