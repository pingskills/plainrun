// Loads the real QML interface offscreen against a temporary database and
// fails on any QML warning (binding errors, missing properties, type errors).
#include "services/appcontroller.h"
#include "ui/theme.h"
#include "ui/uisetup.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>

Q_IMPORT_QML_PLUGIN(PlainRunPlugin)

using namespace plainrun;

class TestUiSmoke : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qputenv("PLAINRUN_NO_OMARCHY", "1");
        configureQuickStyle();
    }

    void mainWindowLoadsWithoutWarnings()
    {
        QTemporaryDir dir;
        AppController app(dir.filePath("plainrun.db"));
        QVERIFY(app.initialize());
        Theme theme;
        registerQmlSingletons(&app, &theme);

        QQmlApplicationEngine engine;
        QList<QQmlError> warnings;
        connect(&engine, &QQmlEngine::warnings, this, [&](const QList<QQmlError> &w) { warnings += w; });

        engine.loadFromModule("PlainRun", "Main");
        QCOMPARE(engine.rootObjects().size(), 1);
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));

        // Empty state, then data arriving, then the editor opening.
        QTest::qWait(50);
        QVERIFY(app.saveRun(-1, app.todayIso(), "5", "28:15", "Smoke test").value("ok").toBool());
        QVERIFY(app.saveRun(-1, app.shiftDate(app.todayIso(), -8), "10", "55:00", "").value("ok").toBool());
        QTest::qWait(50);
        QMetaObject::invokeMethod(window, "addRun");
        QTest::qWait(50);
        QMetaObject::invokeMethod(window, "editRun", Q_ARG(QVariant, QVariant(app.runs()->idAt(0))));
        QTest::qWait(50);
        app.setPeriod(0);
        app.sortBy(AppController::SortPace);
        QTest::qWait(50);

        // Heart rates arriving, then the narrow and short layouts with Trends shown.
        for (int day = 0; day < 3; ++day)
            QVERIFY(app.saveRun(-1, app.shiftDate(app.todayIso(), -day), "5", "27:00", "", "150").value("ok").toBool());
        window->setProperty("selectedRunId", app.runs()->idAt(0));
        window->resize(600, 300);
        QTest::qWait(50);
        QVERIFY(window->property("compact").toBool());
        QVERIFY(window->property("shortWindow").toBool());
        QMetaObject::invokeMethod(window, "showTrendsView");
        QTest::qWait(50);
        QVERIFY(window->property("trendsShown").toBool());

        QStringList messages;
        for (const QQmlError &e : warnings)
            messages << e.toString();
        QVERIFY2(warnings.isEmpty(), qPrintable(messages.join('\n')));
    }
};

QTEST_MAIN(TestUiSmoke)
#include "tst_ui_smoke.moc"
