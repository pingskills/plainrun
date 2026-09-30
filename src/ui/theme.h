#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

namespace plainrun {

// Colours used by the QML interface.
//
// Source, in order of preference:
//  1. Omarchy's active theme (colors.toml), if present and readable. Watched,
//     so PlainRun follows `omarchy theme set` without restarting.
//  2. The Qt/system palette, when it agrees with the system light/dark setting.
//  3. Built-in restrained light and dark palettes.
class Theme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(bool omarchy READ omarchy NOTIFY changed)
    Q_PROPERTY(QString sourceDescription READ sourceDescription NOTIFY changed)
    Q_PROPERTY(QColor background MEMBER m_background NOTIFY changed)
    Q_PROPERTY(QColor surface MEMBER m_surface NOTIFY changed)
    Q_PROPERTY(QColor base MEMBER m_base NOTIFY changed)
    Q_PROPERTY(QColor text MEMBER m_text NOTIFY changed)
    Q_PROPERTY(QColor mutedText MEMBER m_mutedText NOTIFY changed)
    Q_PROPERTY(QColor border MEMBER m_border NOTIFY changed)
    Q_PROPERTY(QColor accent MEMBER m_accent NOTIFY changed)
    Q_PROPERTY(QColor accentText MEMBER m_accentText NOTIFY changed)
    Q_PROPERTY(QColor selection MEMBER m_selection NOTIFY changed)
    Q_PROPERTY(QColor selectionText MEMBER m_selectionText NOTIFY changed)
    Q_PROPERTY(QColor danger MEMBER m_danger NOTIFY changed)
    Q_PROPERTY(QColor chartMuted MEMBER m_chartMuted NOTIFY changed)
    // Base font size in points, from the desktop font settings (fontconfig/GTK/KDE).
    Q_PROPERTY(qreal fontSize READ fontSize CONSTANT)

public:
    explicit Theme(QObject *parent = nullptr);

    bool dark() const { return m_dark; }
    bool omarchy() const { return m_omarchy; }
    QString sourceDescription() const { return m_sourceDescription; }
    qreal fontSize() const;

    Q_INVOKABLE void reload();

signals:
    void changed();

private:
    bool loadOmarchy();
    void loadSystem();
    void loadBuiltIn(bool dark);
    void derive(const QColor &background, const QColor &text, const QColor &accent,
                const QColor &selection, const QColor &danger, bool dark);
    void rewatch();

    bool m_dark = false;
    bool m_omarchy = false;
    QString m_sourceDescription;
    QColor m_background, m_surface, m_base, m_text, m_mutedText, m_border;
    QColor m_accent, m_accentText, m_selection, m_selectionText, m_danger, m_chartMuted;

    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
};

} // namespace plainrun
