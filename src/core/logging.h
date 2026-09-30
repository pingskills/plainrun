#pragma once

#include <QLoggingCategory>

// Categories default to warnings only, so normal operation is quiet.
// Enable diagnostics with:  QT_LOGGING_RULES="plainrun.*=true" plainrun
Q_DECLARE_LOGGING_CATEGORY(lcApp)
Q_DECLARE_LOGGING_CATEGORY(lcDb)
Q_DECLARE_LOGGING_CATEGORY(lcTheme)
