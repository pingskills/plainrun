#pragma once

#include "core/run.h"

#include <QList>
#include <QSqlDatabase>

#include <optional>

namespace plainrun::db {

// Plain functions over an open connection. All writes validate nothing beyond
// the schema's CHECK constraints; callers validate input first.
QList<Run> allRuns(QSqlDatabase db, QString *error = nullptr);
std::optional<Run> findRun(QSqlDatabase db, qint64 id);

// Inserts `run`, filling in id, createdAt and updatedAt. Returns false on failure.
bool insertRun(QSqlDatabase db, Run &run, QString *error = nullptr);
bool updateRun(QSqlDatabase db, Run &run, QString *error = nullptr);
bool deleteRun(QSqlDatabase db, qint64 id, QString *error = nullptr);

// Inserts all runs in a single transaction: either all are stored or none.
bool insertRuns(QSqlDatabase db, QList<Run> &runs, QString *error = nullptr);

} // namespace plainrun::db
