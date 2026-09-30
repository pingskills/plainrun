#include "database/database.h"

#include "core/logging.h"

#include <QAtomicInt>
#include <QCoreApplication>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

namespace plainrun::db {

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("Database", text);
}

} // namespace

QString uniqueConnectionName(const QString &prefix)
{
    static QAtomicInt counter;
    return QStringLiteral("%1-%2").arg(prefix).arg(counter.fetchAndAddRelaxed(1));
}

HeaderInfo readHeader(QSqlDatabase &db)
{
    HeaderInfo info;
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("PRAGMA application_id")) || !q.next()) {
        info.error = q.lastError().text();
        return info;
    }
    info.applicationId = q.value(0).toInt();
    if (!q.exec(QStringLiteral("PRAGMA user_version")) || !q.next()) {
        info.error = q.lastError().text();
        return info;
    }
    info.userVersion = q.value(0).toInt();
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM sqlite_master WHERE type = 'table'")) || !q.next()) {
        info.error = q.lastError().text();
        return info;
    }
    info.hasTables = q.value(0).toInt() > 0;
    info.ok = true;
    return info;
}

Database::Database()
    : m_connectionName(uniqueConnectionName(QStringLiteral("plainrun")))
{
}

Database::~Database()
{
    close();
}

bool Database::fail(const QString &message)
{
    m_lastError = message;
    qCWarning(lcDb).noquote() << message;
    close();
    return false;
}

bool Database::open(const QString &path, Mode mode)
{
    return open(path, mode, migrations());
}

bool Database::open(const QString &path, Mode mode, const QList<Migration> &steps)
{
    close();
    m_path = path;
    m_lastError.clear();

    const bool readOnly = mode == Mode::ReadOnly;
    if (readOnly && !QFileInfo::exists(path))
        return fail(tr("The database %1 does not exist.").arg(path));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    if (!db.isValid())
        return fail(tr("The Qt SQLite driver (QSQLITE) is not available."));
    db.setDatabaseName(path);
    QString options = QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000");
    if (readOnly)
        options += QStringLiteral(";QSQLITE_OPEN_READONLY");
    db.setConnectOptions(options);

    if (!db.open())
        return fail(tr("Could not open the database %1: %2").arg(path, db.lastError().text()));

    const HeaderInfo header = readHeader(db);
    if (!header.ok)
        return fail(tr("%1 is not a readable SQLite database: %2").arg(path, header.error));

    if (header.applicationId != 0 && header.applicationId != ApplicationId)
        return fail(tr("%1 is not a PlainRun database.").arg(path));
    if (header.applicationId == 0 && header.hasTables)
        return fail(tr("%1 is not a PlainRun database.").arg(path));

    const int latest = steps.isEmpty() ? 0 : steps.back().version;
    if (header.userVersion > latest) {
        return fail(tr("%1 was created by a newer version of PlainRun (schema %2; this version supports %3). "
                       "Please update PlainRun.")
                        .arg(path).arg(header.userVersion).arg(latest));
    }

    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));

    if (readOnly) {
        if (header.userVersion < 1)
            return fail(tr("%1 has not been initialised yet.").arg(path));
        return true;
    }

    QString error;
    if (!migrate(db, steps, &error))
        return fail(tr("Could not update the database %1: %2").arg(path, error));

    qCInfo(lcDb).noquote() << "opened" << path << "schema" << userVersion(db);
    return true;
}

void Database::close()
{
    if (!QSqlDatabase::contains(m_connectionName))
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool Database::isOpen() const
{
    return QSqlDatabase::contains(m_connectionName)
        && QSqlDatabase::database(m_connectionName, false).isOpen();
}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(m_connectionName, false);
}

ScopedConnection::ScopedConnection(const QString &path, bool readOnly)
    : m_name(uniqueConnectionName(QStringLiteral("plainrun-scoped")))
{
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
    m_db.setDatabaseName(path);
    QString options = QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000");
    if (readOnly)
        options += QStringLiteral(";QSQLITE_OPEN_READONLY");
    m_db.setConnectOptions(options);
    m_open = m_db.open();
    if (!m_open)
        m_error = m_db.lastError().text();
}

ScopedConnection::~ScopedConnection()
{
    if (m_db.isOpen())
        m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_name);
}

} // namespace plainrun::db
