#include "services/datalocation.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace plainrun {

QString dataDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString databasePath()
{
    return QDir(dataDirectory()).filePath(QLatin1String(DatabaseFileName));
}

bool ensureDataDirectory(QString *error)
{
    const QString dir = dataDirectory();
    if (dir.isEmpty()) {
        if (error)
            *error = QCoreApplication::translate("DataLocation", "No per-user data location is available.");
        return false;
    }
    if (QDir().mkpath(dir))
        return true;
    if (error)
        *error = QCoreApplication::translate("DataLocation", "Could not create the data folder %1.").arg(dir);
    return false;
}

} // namespace plainrun
