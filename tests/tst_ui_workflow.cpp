// Drives the real QML interface with keyboard events only (offscreen), covering
// the primary workflows: add (with Tab order and Ctrl+S), validation, cancel,
// edit (Ctrl+E), delete with confirmation, search (Ctrl+F), period keys, heart
// rate, and the narrow-window Runs | Trends switch (Ctrl+T).
#include "services/appcontroller.h"
#include "ui/theme.h"
#include "ui/uisetup.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>

Q_IMPORT_QML_PLUGIN(PlainRunPlugin)

using namespace plainrun;

class TestUiWorkflow : public QObject
{
    Q_OBJECT

    QTemporaryDir dir;
    std::unique_ptr<AppController> app;
    std::unique_ptr<Theme> theme;
    std::unique_ptr<QQmlApplicationEngine> engine;
    QQuickWindow *window = nullptr;
    QList<QQmlError> warnings;

    QObject *find(const char *name) { return window->findChild<QObject *>(QLatin1String(name)); }
    bool editorOpen() { return find("runEditor")->property("opened").toBool(); }
    QString focusedName() { return window->activeFocusItem() ? window->activeFocusItem()->objectName() : QString(); }

    void key(int k, Qt::KeyboardModifiers m = Qt::NoModifier)
    {
        QTest::keyClick(window, Qt::Key(k), m);
        QTest::qWait(20);
    }
    void type(const QString &text)
    {
        for (const QChar c : text)
            QTest::keyClick(window, c.toLatin1());
        QTest::qWait(20);
    }
    bool waitFor(const std::function<bool()> &condition)
    {
        return QTest::qWaitFor(condition, 2000);
    }

    // Opens the editor with Ctrl+N and fills it purely from the keyboard.
    void addRunByKeyboard(const QString &date, const QString &distance, const QString &duration)
    {
        key(Qt::Key_N, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return editorOpen() && focusedName() == "distanceField"; }));
        // Distance has focus; Shift+Tab past Yesterday and Today to the date.
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        key(Qt::Key_Backtab, Qt::ShiftModifier);
        QCOMPARE(focusedName(), QStringLiteral("dateField"));
        key(Qt::Key_A, Qt::ControlModifier);
        type(date);
        key(Qt::Key_Tab);
        key(Qt::Key_Tab);
        key(Qt::Key_Tab);
        QCOMPARE(focusedName(), QStringLiteral("distanceField"));
        type(distance);
        key(Qt::Key_Tab);
        QCOMPARE(focusedName(), QStringLiteral("durationField"));
        type(duration);
        key(Qt::Key_S, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return !editorOpen(); }));
    }

private slots:
    void initTestCase()
    {
        qputenv("PLAINRUN_NO_OMARCHY", "1");
        QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
        configureQuickStyle();
        app = std::make_unique<AppController>(dir.filePath("plainrun.db"));
        app->setFixedToday(QDate(2026, 9, 30));
        QVERIFY(app->initialize());
        theme = std::make_unique<Theme>();
        registerQmlSingletons(app.get(), theme.get());

        engine = std::make_unique<QQmlApplicationEngine>();
        connect(engine.get(), &QQmlEngine::warnings, this, [this](const QList<QQmlError> &w) { warnings += w; });
        engine->loadFromModule("PlainRun", "Main");
        QCOMPARE(engine->rootObjects().size(), 1);
        window = qobject_cast<QQuickWindow *>(engine->rootObjects().first());
        QVERIFY(window);
        window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(window));
    }

    void addRunsWithKeyboard()
    {
        addRunByKeyboard("2026-09-20", "5", "30:00");
        addRunByKeyboard("2026-09-23", "5", "28:00");
        addRunByKeyboard("2026-09-27", "10", "58:00");
        QCOMPARE(app->totalRunCount(), 3);
        QCOMPARE(app->periodStats().value("distanceText").toString(), QStringLiteral("20.0 km"));
        // The last saved run is selected and the list has focus again.
        QCOMPARE(window->property("selectedRunId").toLongLong(), app->runs()->idAt(0));
    }

    void invalidInputIsNotSavedAndEscapeCancels()
    {
        key(Qt::Key_N, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return editorOpen(); }));
        type("0");
        key(Qt::Key_Tab);
        type("28:75");
        key(Qt::Key_S, Qt::ControlModifier);
        QTest::qWait(50);
        QVERIFY(editorOpen()); // still open, showing the problems
        QCOMPARE(app->totalRunCount(), 3);
        key(Qt::Key_Escape);
        QVERIFY(waitFor([&] { return !editorOpen(); }));
        QCOMPARE(app->totalRunCount(), 3);
    }

    void editWithCtrlE()
    {
        // End selects the oldest run (Run 1, 20 Sep) in newest-first order.
        key(Qt::Key_End);
        const qint64 run1 = window->property("selectedRunId").toLongLong();
        QCOMPARE(app->runDetails(run1).value("dateIso").toString(), QStringLiteral("2026-09-20"));
        key(Qt::Key_E, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return editorOpen() && focusedName() == "distanceField"; }));
        key(Qt::Key_Tab);
        key(Qt::Key_A, Qt::ControlModifier);
        type("29:30");
        key(Qt::Key_S, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return !editorOpen(); }));
        QCOMPARE(app->runDetails(run1).value("durationText").toString(), QStringLiteral("29:30"));
        QCOMPARE(app->runDetails(run1).value("paceText").toString(), QStringLiteral("5:54/km"));
        QCOMPARE(app->periodStats().value("timeExact").toString(), QStringLiteral("1:55:30"));
    }

    void deleteNeedsConfirmation()
    {
        key(Qt::Key_Home);
        key(Qt::Key_Down); // Run 2 (23 Sep)
        const qint64 run2 = window->property("selectedRunId").toLongLong();
        QCOMPARE(app->runDetails(run2).value("dateIso").toString(), QStringLiteral("2026-09-23"));

        QObject *confirm = find("deleteConfirm");
        key(Qt::Key_Delete);
        QVERIFY(waitFor([&] { return confirm->property("opened").toBool(); }));
        key(Qt::Key_Return); // Cancel has focus: nothing deleted
        QVERIFY(waitFor([&] { return !confirm->property("opened").toBool(); }));
        QCOMPARE(app->totalRunCount(), 3);

        key(Qt::Key_Delete);
        QVERIFY(waitFor([&] { return confirm->property("opened").toBool(); }));
        key(Qt::Key_Tab);    // to "Delete"
        key(Qt::Key_Return);
        QVERIFY(waitFor([&] { return app->totalRunCount() == 2; }));
        QVERIFY(!app->runDetails(run2).value("found").toBool());
        QCOMPARE(app->periodStats().value("distanceText").toString(), QStringLiteral("15.0 km"));
    }

    void searchAndPeriodShortcuts()
    {
        QVERIFY(app->saveRun(-1, "2026-09-29", "8", "45:00", "Easy along the bay").value("ok").toBool());
        key(Qt::Key_F, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return focusedName() == "searchField"; }));
        type("BAY");
        QCOMPARE(app->runs()->count(), 1);
        key(Qt::Key_Escape);
        QCOMPARE(app->search(), QString());
        QCOMPARE(app->runs()->count(), 3);
        QVERIFY(focusedName() != "searchField");

        key(Qt::Key_1, Qt::ControlModifier);
        QCOMPARE(app->period(), 0);
        QCOMPARE(app->runs()->count(), 1); // this week: 29 Sep
        key(Qt::Key_4, Qt::ControlModifier);
        QCOMPARE(app->period(), 3);
        QCOMPARE(app->runs()->count(), 3);
    }

    void savedRunOutsideTheViewIsShown()
    {
        key(Qt::Key_1, Qt::ControlModifier); // This week
        addRunByKeyboard("2026-08-01", "3", "18:00");
        QCOMPARE(app->period(), 3); // switched to All so the new run is visible
        const qint64 selected = window->property("selectedRunId").toLongLong();
        QCOMPARE(app->runDetails(selected).value("dateIso").toString(), QStringLiteral("2026-08-01"));
        QVERIFY(app->runs()->indexOfId(selected) >= 0);
    }

    void heartRateByKeyboard()
    {
        key(Qt::Key_N, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return editorOpen() && focusedName() == "distanceField"; }));
        type("5");
        key(Qt::Key_Tab);
        type("27:30");
        key(Qt::Key_Tab);
        QCOMPARE(focusedName(), QStringLiteral("heartRateField"));
        type("145");
        key(Qt::Key_S, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return !editorOpen(); }));
        const qint64 id = window->property("selectedRunId").toLongLong();
        QCOMPARE(app->runDetails(id).value("heartRateText").toString(), QStringLiteral("145 bpm"));
        QCOMPARE(find("selectedHeartRate")->property("text").toString(), QStringLiteral("145 bpm · 798 beats/km"));
    }

    void narrowWindowSwitchesBetweenRunsAndTrends()
    {
        auto *panel = qobject_cast<QQuickItem *>(find("trendsPanel"));
        QVERIFY(panel->isVisible()); // wide: always beside the list
        key(Qt::Key_T, Qt::ControlModifier);
        QVERIFY(!window->property("trendsShown").toBool()); // nothing to switch when wide

        window->resize(600, 500);
        QVERIFY(waitFor([&] { return window->property("compact").toBool(); }));
        QVERIFY(!panel->isVisible());
        QVERIFY(qobject_cast<QQuickItem *>(find("runsViewSwitch"))->isVisible());

        key(Qt::Key_T, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return panel->isVisible(); }));
        QVERIFY(window->property("trendsShown").toBool());
        key(Qt::Key_T, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return !panel->isVisible(); }));

        // Search collapses to a button, and Ctrl+F still works from Trends.
        auto *searchButton = qobject_cast<QQuickItem *>(find("searchButton"));
        QVERIFY(searchButton->isVisible());
        key(Qt::Key_T, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return panel->isVisible(); }));
        key(Qt::Key_F, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return focusedName() == "searchField"; }));
        QVERIFY(!window->property("trendsShown").toBool());
        type("bay");
        QCOMPARE(app->runs()->count(), 1);
        key(Qt::Key_Escape);
        QVERIFY(waitFor([&] { return searchButton->isVisible(); }));
        QCOMPARE(app->search(), QString());

        // A period shortcut from Trends goes back to the runs.
        key(Qt::Key_T, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return panel->isVisible(); }));
        key(Qt::Key_4, Qt::ControlModifier);
        QVERIFY(waitFor([&] { return !panel->isVisible(); }));

        window->resize(1080, 720);
        QVERIFY(waitFor([&] { return panel->isVisible() && !window->property("compact").toBool(); }));
    }

    void noQmlWarnings()
    {
        QStringList messages;
        for (const QQmlError &e : warnings)
            messages << e.toString();
        QVERIFY2(warnings.isEmpty(), qPrintable(messages.join('\n')));
    }

    void cleanupTestCase()
    {
        engine.reset();
    }
};

QTEST_MAIN(TestUiWorkflow)
#include "tst_ui_workflow.moc"
