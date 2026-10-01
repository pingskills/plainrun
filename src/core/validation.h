#pragma once

#include <QDate>
#include <QString>

namespace plainrun {

inline constexpr qint64 MaxDistanceMetres = 1'000'000;      // 1000 km
inline constexpr qint64 MaxDurationSeconds = 7 * 24 * 3600;  // one week
inline const QDate EarliestRunDate{1900, 1, 1};
inline constexpr int MinHeartRateBpm = 30;
inline constexpr int MaxHeartRateBpm = 250;

struct RunValidation {
    QDate date;
    qint64 distanceMetres = 0;
    qint64 durationSeconds = 0;
    int heartRateBpm = 0; // 0 when left blank
    QString dateError;
    QString distanceError;
    QString durationError;
    QString heartRateError;

    bool ok() const
    {
        return dateError.isEmpty() && distanceError.isEmpty() && durationError.isEmpty()
            && heartRateError.isEmpty();
    }
};

// Validates the text fields of the run form. `today` is the local date used to
// reject runs in the future. Heart rate is optional and may be left blank.
RunValidation validateRunInput(const QString &date, const QString &distance,
                               const QString &duration, const QString &heartRate,
                               const QDate &today);

// Validates already-parsed values (used by CSV import). Returns an empty
// string when valid, otherwise a short explanation. heartRateBpm 0 = none.
QString validateRunValues(const QDate &date, qint64 distanceMetres,
                          qint64 durationSeconds, int heartRateBpm, const QDate &today);

// Normalises a note for storage: trims surrounding whitespace.
QString normaliseNote(const QString &note);

} // namespace plainrun
