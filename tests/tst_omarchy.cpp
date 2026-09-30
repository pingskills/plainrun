#include "omarchy/omarchycolors.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace plainrun;

class TestOmarchy : public QObject
{
    Q_OBJECT

private slots:
    void parsesColorsToml()
    {
        const QString text = "mode = \"dark\"\n"
                             "# comment\n"
                             "accent = \"#7aa2f7\"   # trailing comment\n"
                             "background='#1a1b26'\n"
                             "\n"
                             "[section]\n"
                             "broken line\n"
                             "Foreground = \"#a9b1d6\"\n";
        const auto values = omarchy::parseColorsToml(text);
        QCOMPARE(values.value("mode"), QStringLiteral("dark"));
        QCOMPARE(values.value("accent"), QStringLiteral("#7aa2f7"));
        QCOMPARE(values.value("background"), QStringLiteral("#1a1b26"));
        QCOMPARE(values.value("foreground"), QStringLiteral("#a9b1d6"));
        QCOMPARE(values.size(), 4);
    }

    void emptyOrGarbageInputIsHarmless()
    {
        QVERIFY(omarchy::parseColorsToml("").isEmpty());
        QVERIFY(omarchy::parseColorsToml("\x01\x02 = = =\n[[[").isEmpty());
    }

    void locatesActiveThemeViaXdgDirs()
    {
        QTemporaryDir dir;
        qputenv("XDG_STATE_HOME", dir.filePath("state").toUtf8());
        qputenv("XDG_CONFIG_HOME", dir.filePath("config").toUtf8());
        QVERIFY(omarchy::activeColorsFile().isEmpty()); // absent: no crash, no result

        const QString themeDir = dir.filePath("config/omarchy/current/theme");
        QVERIFY(QDir().mkpath(themeDir));
        QFile f(themeDir + "/colors.toml");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("background = \"#ffffff\"\n");
        f.close();
        QCOMPARE(omarchy::activeColorsFile(), themeDir + "/colors.toml");

        const QString stateTheme = dir.filePath("state/omarchy/current/theme");
        QVERIFY(QDir().mkpath(stateTheme));
        QFile g(stateTheme + "/colors.toml");
        QVERIFY(g.open(QIODevice::WriteOnly));
        g.close();
        QCOMPARE(omarchy::activeColorsFile(), stateTheme + "/colors.toml"); // Omarchy 4 location preferred
    }

    void canBeDisabled()
    {
        qputenv("PLAINRUN_NO_OMARCHY", "1");
        QVERIFY(omarchy::disabledByEnvironment());
        qputenv("PLAINRUN_NO_OMARCHY", "0");
        QVERIFY(!omarchy::disabledByEnvironment());
        qunsetenv("PLAINRUN_NO_OMARCHY");
        QVERIFY(!omarchy::disabledByEnvironment());
    }
};

QTEST_GUILESS_MAIN(TestOmarchy)
#include "tst_omarchy.moc"
