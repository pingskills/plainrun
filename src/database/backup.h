#pragma once

#include <QDate>
#include <QString>

namespace plainrun::db {

class Database;

// A PlainRun backup is a standalone SQLite database file with the PlainRun
// application_id in its header and a schema version this build understands.
// It is produced with SQLite's VACUUM INTO, which writes a transactionally
// consistent, compacted snapshot even while the source connection is open.
struct BackupInfo {
    bool valid = false;
    QString error;        // user-facing reason when !valid
    int schemaVersion = 0;
    int runCount = 0;
    QDate latestRun;
};

// Validates a candidate backup without modifying it.
BackupInfo inspectBackup(const QString &path);

// Writes a snapshot of `db` to `targetPath`, replacing that file atomically if
// it exists. The snapshot is verified before it is moved into place.
bool writeBackup(Database &db, const QString &targetPath, QString *error);

// Replaces the live database with the backup at `sourcePath`:
//   1. validate the backup;
//   2. copy it into the data directory as a temporary file and migrate the copy
//      to the current schema if it is older;
//   3. save the current database as plainrun-pre-restore-<timestamp>.db;
//   4. close the live connection, atomically rename the copy over plainrun.db;
//   5. reopen.
// If anything fails before step 4 the live database is untouched. If reopening
// fails after step 4, the pre-restore copy is put back.
// `safetyCopyPath` receives the location of the preserved current data.
bool restoreBackup(Database &db, const QString &sourcePath, QString *safetyCopyPath, QString *error);

} // namespace plainrun::db
