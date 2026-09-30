#include "ui/uisetup.h"

#include "services/appcontroller.h"
#include "ui/theme.h"

#include <QQmlEngine>
#include <QQuickStyle>

namespace plainrun {

void configureQuickStyle()
{
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(QStringLiteral("Fusion"));
}

void registerQmlSingletons(AppController *controller, Theme *theme)
{
    qmlRegisterSingletonInstance("PlainRun.Core", 1, 0, "App", controller);
    qmlRegisterSingletonInstance("PlainRun.Core", 1, 0, "Theme", theme);
    qmlRegisterUncreatableType<RunListModel>("PlainRun.Core", 1, 0, "RunListModel",
                                             QStringLiteral("Provided by App.runs"));
}

} // namespace plainrun
