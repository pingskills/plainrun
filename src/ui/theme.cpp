#include "ui/theme.h"

#include "core/logging.h"
#include "omarchy/omarchycolors.h"

#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

#include <algorithm>
#include <cmath>

namespace plainrun {

namespace {

QColor mix(const QColor &a, const QColor &b, qreal amountOfB)
{
    const qreal t = std::clamp(amountOfB, 0.0, 1.0);
    return QColor::fromRgbF(a.redF() * (1 - t) + b.redF() * t, a.greenF() * (1 - t) + b.greenF() * t,
                            a.blueF() * (1 - t) + b.blueF() * t);
}

qreal luminance(const QColor &c)
{
    auto channel = [](qreal v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel(c.redF()) + 0.7152 * channel(c.greenF()) + 0.0722 * channel(c.blueF());
}

qreal contrast(const QColor &a, const QColor &b)
{
    const qreal la = luminance(a), lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

QColor readableOn(const QColor &background, const QColor &preferredA, const QColor &preferredB)
{
    return contrast(background, preferredA) >= contrast(background, preferredB) ? preferredA : preferredB;
}

QColor colorValue(const QHash<QString, QString> &values, const QString &key)
{
    const QString v = values.value(key);
    return v.isEmpty() ? QColor() : QColor::fromString(v);
}

} // namespace

Theme::Theme(QObject *parent)
    : QObject(parent)
{
    m_reloadTimer.setSingleShot(true);
    m_reloadTimer.setInterval(250); // theme switches write several files; settle first
    connect(&m_reloadTimer, &QTimer::timeout, this, &Theme::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_reloadTimer, qOverload<>(&QTimer::start));
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_reloadTimer, qOverload<>(&QTimer::start));
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        if (!m_omarchy)
            reload();
    });
    reload();
}

qreal Theme::fontSize() const
{
    const QFont f = QGuiApplication::font();
    if (f.pointSizeF() > 0)
        return f.pointSizeF();
    return f.pixelSize() > 0 ? f.pixelSize() * 0.75 : 10.0;
}

void Theme::reload()
{
    // PLAINRUN_COLOR_SCHEME=light|dark forces PlainRun's built-in palettes.
    const QString forced = qEnvironmentVariable("PLAINRUN_COLOR_SCHEME").toLower();
    if (forced == QLatin1String("light") || forced == QLatin1String("dark")) {
        m_omarchy = false;
        loadBuiltIn(forced == QLatin1String("dark"));
    } else if (!loadOmarchy()) {
        loadSystem();
    }
    rewatch();
    emit changed();
}

bool Theme::loadOmarchy()
{
    m_omarchy = false;
    if (omarchy::disabledByEnvironment())
        return false;
    const QString path = omarchy::activeColorsFile();
    if (path.isEmpty())
        return false;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCInfo(lcTheme) << "cannot read" << path << file.errorString();
        return false;
    }
    const auto values = omarchy::parseColorsToml(QString::fromUtf8(file.readAll()));
    const QColor bg = colorValue(values, QStringLiteral("background"));
    const QColor fg = colorValue(values, QStringLiteral("foreground"));
    if (!bg.isValid() || !fg.isValid()) {
        qCInfo(lcTheme) << "incomplete Omarchy colours in" << path << "- using system colours";
        return false;
    }
    QColor accent = colorValue(values, QStringLiteral("accent"));
    if (!accent.isValid())
        accent = colorValue(values, QStringLiteral("blue"));
    if (!accent.isValid())
        accent = fg;
    const QString mode = values.value(QStringLiteral("mode")).toLower();
    const bool dark = mode == QLatin1String("light") ? false
        : mode == QLatin1String("dark")              ? true
                                                     : bg.lightnessF() < 0.5;

    derive(bg, fg, accent, colorValue(values, QStringLiteral("selection")),
           colorValue(values, QStringLiteral("red")), dark);

    QString themeName;
    QFile nameFile(QFileInfo(path).absolutePath() + QStringLiteral("/../theme.name"));
    if (nameFile.open(QIODevice::ReadOnly | QIODevice::Text))
        themeName = QString::fromUtf8(nameFile.readAll()).trimmed();
    m_omarchy = true;
    m_sourceDescription = themeName.isEmpty() ? tr("Omarchy theme") : tr("Omarchy theme: %1").arg(themeName);
    qCInfo(lcTheme).noquote() << "using" << m_sourceDescription << "from" << path;
    return true;
}

void Theme::loadSystem()
{
    const QPalette pal = QGuiApplication::palette();
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    const bool paletteDark = pal.color(QPalette::Window).lightnessF() < 0.5;
    const bool wantDark = scheme == Qt::ColorScheme::Dark || (scheme == Qt::ColorScheme::Unknown && paletteDark);

    if (paletteDark == wantDark) {
        derive(pal.color(QPalette::Window), pal.color(QPalette::WindowText), pal.color(QPalette::Highlight),
               QColor(), QColor(), wantDark);
        m_sourceDescription = tr("System colours");
    } else {
        loadBuiltIn(wantDark);
        return;
    }
    qCInfo(lcTheme).noquote() << "using" << m_sourceDescription;
}

void Theme::loadBuiltIn(bool dark)
{
    if (dark) {
        derive(QColor(0x1d, 0x1f, 0x21), QColor(0xd8, 0xda, 0xdc), QColor(0x6e, 0xa8, 0xdc), QColor(), QColor(), true);
        m_sourceDescription = tr("Built-in dark colours");
    } else {
        derive(QColor(0xfb, 0xfb, 0xfa), QColor(0x1f, 0x23, 0x28), QColor(0x2f, 0x6d, 0xb3), QColor(), QColor(), false);
        m_sourceDescription = tr("Built-in light colours");
    }
    qCInfo(lcTheme).noquote() << "using" << m_sourceDescription;
}

void Theme::derive(const QColor &background, const QColor &text, const QColor &accent,
                   const QColor &selection, const QColor &danger, bool dark)
{
    m_dark = dark;
    m_background = background;
    m_text = text;
    m_surface = mix(background, text, 0.045);
    // Input fields: slightly lifted from the window in dark themes, whiter in light ones.
    m_base = dark ? mix(background, text, 0.06) : mix(background, QColor(Qt::white), 0.7);
    m_mutedText = mix(text, background, 0.38);
    // Keep secondary text comfortably readable (WCAG AA for normal text).
    for (qreal t = 0.38; contrast(m_mutedText, background) < 4.5 && t > 0; t -= 0.04)
        m_mutedText = mix(text, background, t);
    m_border = mix(background, text, 0.16);
    m_accent = accent;
    m_accentText = readableOn(accent, background, text);
    if (contrast(accent, m_accentText) < 3.0)
        m_accentText = readableOn(accent, Qt::black, Qt::white);
    m_selection = selection.isValid() ? selection : mix(background, accent, dark ? 0.28 : 0.18);
    m_selectionText = text;
    m_danger = danger.isValid() ? danger : (dark ? QColor(0xf2, 0x7a, 0x7a) : QColor(0xb4, 0x23, 0x18));
    m_chartMuted = mix(background, accent, 0.5);
}

void Theme::rewatch()
{
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());
    if (omarchy::disabledByEnvironment())
        return;

    // Watch the colours file and the directories above it: Omarchy replaces the
    // theme directory when switching themes, which drops file watches.
    QStringList paths;
    for (const QString &candidate : omarchy::colorsFileCandidates()) {
        const QFileInfo file(candidate);
        const QString themeDir = file.absolutePath();
        const QString currentDir = QFileInfo(themeDir).absolutePath();
        for (const QString &p : {candidate, themeDir, currentDir}) {
            if (QFileInfo::exists(p))
                paths << p;
        }
    }
    if (!paths.isEmpty())
        m_watcher.addPaths(paths);
}

} // namespace plainrun
