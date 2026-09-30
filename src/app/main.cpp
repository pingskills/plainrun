#include "core/logging.h"
#include "plainrun_config.h"
#include "services/appcontroller.h"
#include "services/cli.h"
#include "services/datalocation.h"
#include "ui/theme.h"
#include "ui/uisetup.h"

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

Q_IMPORT_QML_PLUGIN(PlainRunPlugin)

namespace {

void setIdentity()
{
    // applicationName determines the XDG data directory: ~/.local/share/plainrun
    QCoreApplication::setApplicationName(QStringLiteral("plainrun"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PLAINRUN_VERSION));
}

QString settingsPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
        .filePath(QStringLiteral("plainrun.conf"));
}

} // namespace

int main(int argc, char *argv[])
{
    if (plainrun::cli::isCliInvocation(argc, argv)) {
        QCoreApplication app(argc, argv);
        setIdentity();
        QTextStream out(stdout);
        QTextStream err(stderr);
        return plainrun::cli::run(app.arguments(), plainrun::databasePath(), out, err, QDate::currentDate());
    }

    QGuiApplication app(argc, argv);
    setIdentity();
    QGuiApplication::setApplicationDisplayName(QStringLiteral("PlainRun"));
    // Matches the .desktop file name; used as the Wayland app_id.
    QGuiApplication::setDesktopFileName(QStringLiteral(PLAINRUN_APP_ID));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral(PLAINRUN_APP_ID),
        QIcon(QStringLiteral(":/qt/qml/PlainRun/resources/icons/png/256/" PLAINRUN_APP_ID ".png"))));

    plainrun::configureQuickStyle();

    plainrun::AppController controller(plainrun::databasePath());
    controller.initialize();

    // Remember only the chosen history view between sessions.
    QSettings settings(settingsPath(), QSettings::IniFormat);
    controller.setPeriod(settings.value(QStringLiteral("history/period"), 3).toInt());
    QObject::connect(&controller, &plainrun::AppController::viewChanged, &controller, [&] {
        settings.setValue(QStringLiteral("history/period"), controller.period());
    });

    plainrun::Theme theme;
    plainrun::registerQmlSingletons(&controller, &theme);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("PlainRun"), QStringLiteral("Main"));
    return QGuiApplication::exec();
}
