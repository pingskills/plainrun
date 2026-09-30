#include "core/runformat.h"

#include <QRegularExpression>

namespace plainrun {

namespace {

bool allDigits(QStringView s)
{
    if (s.isEmpty())
        return false;
    for (QChar c : s) {
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            return false;
    }
    return true;
}

} // namespace

std::optional<qint64> parseDurationSeconds(const QString &text)
{
    const QString s = text.trimmed();
    if (s.isEmpty())
        return std::nullopt;

    const QStringList parts = s.split(QLatin1Char(':'));
    for (const QString &p : parts) {
        if (!allDigits(p))
            return std::nullopt;
    }

    if (parts.size() == 1) {
        if (parts[0].size() > 4)
            return std::nullopt;
        return parts[0].toLongLong() * 60;
    }
    if (parts.size() == 2) {
        if (parts[0].size() > 4 || parts[1].size() != 2)
            return std::nullopt;
        const qint64 m = parts[0].toLongLong();
        const qint64 sec = parts[1].toLongLong();
        if (sec >= 60)
            return std::nullopt;
        return m * 60 + sec;
    }
    if (parts.size() == 3) {
        if (parts[0].size() > 3 || parts[1].size() != 2 || parts[2].size() != 2)
            return std::nullopt;
        const qint64 h = parts[0].toLongLong();
        const qint64 m = parts[1].toLongLong();
        const qint64 sec = parts[2].toLongLong();
        if (m >= 60 || sec >= 60)
            return std::nullopt;
        return h * 3600 + m * 60 + sec;
    }
    return std::nullopt;
}

std::optional<qint64> parseDistanceMetres(const QString &text)
{
    QString s = text.trimmed();
    if (s.endsWith(QLatin1String("km"), Qt::CaseInsensitive))
        s = s.chopped(2).trimmed();
    s.replace(QLatin1Char(','), QLatin1Char('.'));
    if (s.isEmpty())
        return std::nullopt;

    const qsizetype dot = s.indexOf(QLatin1Char('.'));
    QString whole = dot < 0 ? s : s.left(dot);
    QString frac = dot < 0 ? QString() : s.mid(dot + 1);
    if (whole.isEmpty() && frac.isEmpty())
        return std::nullopt;
    if (!whole.isEmpty() && !allDigits(whole))
        return std::nullopt;
    if (!frac.isEmpty() && !allDigits(frac))
        return std::nullopt;
    if (whole.size() > 6 || frac.size() > 9)
        return std::nullopt;

    // Exact decimal conversion: first three fractional digits are metres,
    // the fourth decides rounding.
    const QString padded = (frac + QStringLiteral("0000")).left(4);
    qint64 metres = whole.toLongLong() * 1000 + padded.left(3).toLongLong();
    if (padded.at(3) >= QLatin1Char('5'))
        ++metres;
    return metres;
}

std::optional<QDate> parseIsoDate(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^\\d{4}-\\d{2}-\\d{2}$"));
    const QString s = text.trimmed();
    if (!re.match(s).hasMatch())
        return std::nullopt;
    const QDate d = QDate::fromString(s, Qt::ISODate);
    if (!d.isValid())
        return std::nullopt;
    return d;
}

qint64 paceSecondsPerKm(qint64 distanceMetres, qint64 durationSeconds)
{
    if (distanceMetres <= 0 || durationSeconds <= 0)
        return 0;
    // Integer arithmetic, rounding half up.
    return (durationSeconds * 1000 + distanceMetres / 2) / distanceMetres;
}

QString formatDuration(qint64 seconds)
{
    if (seconds < 0)
        seconds = 0;
    const qint64 h = seconds / 3600;
    const qint64 m = (seconds % 3600) / 60;
    const qint64 s = seconds % 60;
    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}

QString formatDurationShort(qint64 seconds)
{
    if (seconds < 0)
        seconds = 0;
    const qint64 totalMinutes = (seconds + 30) / 60;
    const qint64 h = totalMinutes / 60;
    const qint64 m = totalMinutes % 60;
    if (h > 0)
        return QStringLiteral("%1h %2m").arg(h).arg(m, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1m").arg(m);
}

QString formatPace(qint64 secondsPerKm)
{
    if (secondsPerKm <= 0)
        return QStringLiteral("–");
    return QStringLiteral("%1:%2")
        .arg(secondsPerKm / 60)
        .arg(secondsPerKm % 60, 2, 10, QLatin1Char('0'));
}

QString formatKm(qint64 metres, int decimals, const QLocale &locale)
{
    return locale.toString(static_cast<double>(metres) / 1000.0, 'f', decimals);
}

QString formatIsoDate(const QDate &date)
{
    return date.toString(Qt::ISODate);
}

} // namespace plainrun
