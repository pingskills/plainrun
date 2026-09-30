#pragma once

#include <QDate>

namespace plainrun {

enum class Period { Week = 0, Month = 1, Year = 2, All = 3 };

// Inclusive date range. An invalid bound means "unbounded" on that side.
struct DateRange {
    QDate first;
    QDate last;

    bool contains(const QDate &d) const
    {
        return (!first.isValid() || d >= first) && (!last.isValid() || d <= last);
    }
    bool operator==(const DateRange &) const = default;
};

// Calendar weeks start on Monday (ISO 8601), independent of locale.
QDate startOfWeek(const QDate &date);

DateRange weekContaining(const QDate &date);
DateRange monthContaining(const QDate &date);
DateRange yearContaining(const QDate &date);
DateRange previousWeek(const QDate &today);
DateRange previousMonth(const QDate &today);

// Range for one of the history views relative to `today`.
DateRange rangeFor(Period period, const QDate &today);

} // namespace plainrun
