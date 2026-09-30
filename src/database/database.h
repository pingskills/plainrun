#pragma once

#include "database/migrations.h"

#include <QSqlDatabase>
#include <QString>

namespace plainrun::db {

// Owns one named Qt SQL connection to a PlainRun SQLite database.
class Database
{
public:
    // Stored in the SQLite header (PRAGMA application_id) to identify PlainRun
    // databases and backups. ASCII "PlRn".
    static constexpr int ApplicationId = 0x506C526E;

    enum class Mode { ReadWrite, ReadOnly };

    Database();
    ~Database();
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // ReadWrite: creates the file if needed and migrates it to the latest schema.
    // ReadOnly: never creates or modifies the file.
    // Refuses (returns false, sets lastError) databases that are not PlainRun
    // databases or were written by a newer PlainRun. Never deletes or recreates
    // an existing file.
    bool open(const QString &path, Mode mode = Mode::ReadWrite);
    bool open(const QString &path, Mode mode, const QList<Migration> &steps);
    void close();

    bool isOpen() const;
    QSqlDatabase connection() const;
    QString path() const { return m_path; }
    QString lastError() const { return m_lastError; }

private:
    bool fail(const QString &message);

    QString m_connectionName;
    QString m_path;
    QString m_lastError;
};

// Opens a short-lived connection for inspecting or copying a file.
// The connection is removed when the object goes out of scope.
class ScopedConnection
{
public:
    ScopedConnection(const QString &path, bool readOnly);
    ~ScopedConnection();
    ScopedConnection(const ScopedConnection &) = delete;
    ScopedConnection &operator=(const ScopedConnection &) = delete;

    bool isOpen() const { return m_open; }
    QSqlDatabase &db() { return m_db; }
    QString errorText() const { return m_error; }

private:
    QString m_name;
    QSqlDatabase m_db;
    bool m_open = false;
    QString m_error;
};

QString uniqueConnectionName(const QString &prefix);

// Reads the header identification of an open connection.
struct HeaderInfo {
    bool ok = false;
    int applicationId = 0;
    int userVersion = 0;
    bool hasTables = false;
    QString error;
};
HeaderInfo readHeader(QSqlDatabase &db);

} // namespace plainrun::db
