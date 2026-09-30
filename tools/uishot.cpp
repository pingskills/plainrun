// Development only (built with -DPLAINRUN_BUILD_DEVTOOLS=ON, never installed).
// Renders the real QML interface offscreen and saves a PNG, for reviewing the
// layout at different sizes, themes and scale factors without a desktop.
//
//   QT_QPA_PLATFORM=offscreen plainrun-uishot <db> <out.png> [width height] [select|editor|add|none]
#include "services/appcontroller.h"
#include "ui/theme.h"
#include "ui/uisetup.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickWindow>
#include <QTimer>

Q_IMPORT_QML_PLUGIN(PlainRunPlugin)

using namespace plainrun;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("plainrun"));
    const QStringList args = app.arguments();
    if (args.size() < 3)
        return 2;
    const int width = args.value(3, QStringLiteral("1080")).toInt();
    const int height = args.value(4, QStringLiteral("720")).toInt();
    const QString mode = args.value(5, QStringLiteral("none"));

    configureQuickStyle();
    AppController controller(args.at(1));
    controller.initialize();
    Theme theme;
    registerQmlSingletons(&controller, &theme);

    QQmlApplicationEngine engine;
    engine.loadFromModule(QStringLiteral("PlainRun"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty())
        return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    window->resize(width, height);

    QTimer::singleShot(200, [&] {
        if (mode == QLatin1String("select") || mode == QLatin1String("editor"))
            window->setProperty("selectedRunId", controller.runs()->idAt(0));
        if (mode == QLatin1String("editor"))
            QMetaObject::invokeMethod(window, "editRun", Q_ARG(QVariant, QVariant(controller.runs()->idAt(0))));
        if (mode == QLatin1String("add"))
            QMetaObject::invokeMethod(window, "addRun");
    });
    QTimer::singleShot(900, [&] {
        window->grabWindow().save(args.at(2));
        QCoreApplication::quit();
    });
    return QGuiApplication::exec();
}
