#include "database/backup.h"

#include "core/logging.h"
#include "database/database.h"
#include "database/migrations.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

#include <filesystem>
#include <system_error>

namespace plainrun::db {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("Backup", text);
}

bool vacuumInto(QSqlDatabase db, const QString &target, QString *error)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral("VACUUM INTO ?"));
    q.addBindValue(target);
    if (!q.exec()) {
        if (error)
            *error = q.lastError().text();
        return false;
    }
    return true;
}

bool replaceFile(const QString &from, const QString &to, QString *error)
{
    std::error_code ec;
    std::filesystem::rename(QFile::encodeName(from).toStdString(), QFile::encodeName(to).toStdString(), ec);
    if (ec) {
        if (error)
            *error = QString::fromStdString(ec.message());
        return false;
    }
    return true;
}

void removeIfExists(const QString &path)
{
    if (QFileInfo::exists(path))
        QFile::remove(path);
}

} // namespace

BackupInfo inspectBackup(const QString &path)
{
    BackupInfo info;
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        info.error = tr("The file does not exist.");
        return info;
    }
    if (!fi.isReadable()) {
        info.error = tr("The file cannot be read.");
        return info;
    }
    if (fi.size() == 0) {
        info.error = tr("The file is empty, so it is not a PlainRun backup.");
        return info;
    }

    ScopedConnection conn(path, true);
    if (!conn.isOpen()) {
        info.error = tr("The file could not be opened: %1").arg(conn.errorText());
        return info;
    }

    const HeaderInfo header = readHeader(conn.db());
    if (!header.ok) {
        qCInfo(lcDb).noquote() << "backup header unreadable:" << header.error;
        info.error = tr("The file is not a SQLite database, so it is not a PlainRun backup.");
        return info;
    }
    if (header.applicationId != Database::ApplicationId) {
        info.error = tr("The file is a database, but not a PlainRun backup.");
        return info;
    }
    if (header.userVersion < 1) {
        info.error = tr("The backup is incomplete (no PlainRun data).");
        return info;
    }
    if (header.userVersion > latestSchemaVersion()) {
        info.error = tr("The backup was made by a newer version of PlainRun. Please update PlainRun first.");
        return info;
    }

    QSqlQuery q(conn.db());
    if (!q.exec(QStringLiteral("PRAGMA quick_check")) || !q.next()
        || q.value(0).toString() != QLatin1String("ok")) {
        info.error = tr("The backup is damaged (SQLite integrity check failed).");
        return info;
    }
    if (!q.exec(QStringLiteral("SELECT COUNT(*), MAX(run_date) FROM runs")) || !q.next()) {
        info.error = tr("The backup does not contain a readable runs table.");
        return info;
    }
    info.runCount = q.value(0).toInt();
    info.latestRun = QDate::fromString(q.value(1).toString(), Qt::ISODate);
    info.schemaVersion = header.userVersion;
    info.valid = true;
    return info;
}

bool writeBackup(Database &db, const QString &targetPath, QString *error)
{
    if (!db.isOpen()) {
        if (error)
            *error = tr("The database is not open.");
        return false;
    }

    const QString temp = targetPath + QStringLiteral(".partial");
    removeIfExists(temp);

    QString sqlError;
    if (!vacuumInto(db.connection(), temp, &sqlError)) {
        qCWarning(lcDb).noquote() << "backup failed:" << sqlError;
        removeIfExists(temp);
        if (error)
            *error = tr("Could not write the backup: %1").arg(sqlError);
        return false;
    }

    const BackupInfo check = inspectBackup(temp);
    if (!check.valid) {
        removeIfExists(temp);
        if (error)
            *error = tr("The backup could not be verified: %1").arg(check.error);
        return false;
    }

    QString fsError;
    if (!replaceFile(temp, targetPath, &fsError)) {
        removeIfExists(temp);
        if (error)
            *error = tr("Could not save the backup: %1").arg(fsError);
        return false;
    }
    qCInfo(lcDb).noquote() << "backup written:" << targetPath << check.runCount << "runs";
    return true;
}

bool restoreBackup(Database &db, const QString &sourcePath, QString *safetyCopyPath, QString *error)
{
    auto failWith = [error](const QString &message) {
        qCWarning(lcDb).noquote() << "restore failed:" << message;
        if (error)
            *error = message;
        return false;
    };

    const QString livePath = db.path();
    if (livePath.isEmpty() || !db.isOpen())
        return failWith(tr("The database is not open."));

    const BackupInfo info = inspectBackup(sourcePath);
    if (!info.valid)
        return failWith(info.error);

    const QDir dataDir = QFileInfo(livePath).absoluteDir();
    const QString staged = dataDir.filePath(QStringLiteral("plainrun.db.restoring"));
    removeIfExists(staged);

    // 1. Copy the backup into the data directory as a clean, standalone file.
    {
        ScopedConnection source(sourcePath, true);
        if (!source.isOpen())
            return failWith(tr("Could not open the backup: %1").arg(source.errorText()));
        QString sqlError;
        if (!vacuumInto(source.db(), staged, &sqlError)) {
            removeIfExists(staged);
            return failWith(tr("Could not copy the backup: %1").arg(sqlError));
        }
    }

    // 2. Bring the copy up to the current schema (no-op if already current).
    {
        Database stagedDb;
        if (!stagedDb.open(staged, Database::Mode::ReadWrite)) {
            const QString message = stagedDb.lastError();
            stagedDb.close();
            removeIfExists(staged);
            return failWith(tr("Could not prepare the backup: %1").arg(message));
        }
    }

    // 3. Preserve the current data before touching it.
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd-HHmmss"));
    QString safety = dataDir.filePath(QStringLiteral("plainrun-pre-restore-%1.db").arg(stamp));
    for (int i = 2; QFileInfo::exists(safety); ++i)
        safety = dataDir.filePath(QStringLiteral("plainrun-pre-restore-%1-%2.db").arg(stamp).arg(i));
    {
        QString sqlError;
        if (!vacuumInto(db.connection(), safety, &sqlError)) {
            removeIfExists(safety);
            removeIfExists(staged);
            return failWith(tr("Could not save a copy of your current data, so nothing was changed: %1")
                                .arg(sqlError));
        }
    }

    // 4. Swap files while no connection is open.
    db.close();
    QString fsError;
    if (!replaceFile(staged, livePath, &fsError)) {
        removeIfExists(staged);
        db.open(livePath);
        return failWith(tr("Could not replace the database: %1. Your data is unchanged.").arg(fsError));
    }
    removeIfExists(livePath + QStringLiteral("-journal"));

    // 5. Reopen; on failure put the preserved data back.
    if (!db.open(livePath)) {
        const QString openError = db.lastError();
        QFile::remove(livePath);
        if (QFile::copy(safety, livePath))
            db.open(livePath);
        return failWith(tr("The restored database could not be opened (%1). Your previous data was put back.")
                            .arg(openError));
    }

    if (safetyCopyPath)
        *safetyCopyPath = safety;
    qCInfo(lcDb).noquote() << "restored" << info.runCount << "runs; previous data kept at" << safety;
    return true;
}

} // namespace plainrun::db
