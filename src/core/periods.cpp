#include "core/periods.h"

namespace plainrun {

QDate startOfWeek(const QDate &date)
{
    return date.addDays(1 - date.dayOfWeek()); // dayOfWeek(): Monday = 1
}

DateRange weekContaining(const QDate &date)
{
    const QDate first = startOfWeek(date);
    return {first, first.addDays(6)};
}

DateRange monthContaining(const QDate &date)
{
    const QDate first(date.year(), date.month(), 1);
    return {first, first.addMonths(1).addDays(-1)};
}

DateRange yearContaining(const QDate &date)
{
    return {QDate(date.year(), 1, 1), QDate(date.year(), 12, 31)};
}

DateRange previousWeek(const QDate &today)
{
    return weekContaining(startOfWeek(today).addDays(-7));
}

DateRange previousMonth(const QDate &today)
{
    return monthContaining(QDate(today.year(), today.month(), 1).addMonths(-1));
}

DateRange rangeFor(Period period, const QDate &today)
{
    switch (period) {
    case Period::Week:
        return weekContaining(today);
    case Period::Month:
        return monthContaining(today);
    case Period::Year:
        return yearContaining(today);
    case Period::All:
        break;
    }
    return {};
}

} // namespace plainrun
