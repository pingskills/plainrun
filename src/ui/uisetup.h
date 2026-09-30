#pragma once

namespace plainrun {

class AppController;
class Theme;

// Selects the Qt Quick Controls style (Fusion, unless the user chose one via
// QT_QUICK_CONTROLS_STYLE). Must run before the QML engine is created.
void configureQuickStyle();

// Exposes the controller and theme to QML as singletons in "PlainRun.Core".
void registerQmlSingletons(AppController *controller, Theme *theme);

} // namespace plainrun
