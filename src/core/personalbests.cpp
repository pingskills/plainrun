#include "core/personalbests.h"

#include <QCoreApplication>

#include <optional>

namespace plainrun {

const QList<PbTarget> &pbTargets()
{
    static const QList<PbTarget> targets = {
        {QCoreApplication::translate("PersonalBests", "1 km"), 1000.0},
        {QCoreApplication::translate("PersonalBests", "5 km"), 5000.0},
        {QCoreApplication::translate("PersonalBests", "10 km"), 10000.0},
        {QCoreApplication::translate("PersonalBests", "Half marathon"), 21097.5},
        {QCoreApplication::translate("PersonalBests", "Marathon"), 42195.0},
    };
    return targets;
}

bool qualifiesFor(qint64 distanceMetres, double targetMetres)
{
    const double d = static_cast<double>(distanceMetres);
    return d >= targetMetres * PbLowerTolerance && d <= targetMetres * PbUpperTolerance;
}

QList<PersonalBest> personalBests(const QList<Run> &runs)
{
    QList<PersonalBest> result;
    for (const PbTarget &target : pbTargets()) {
        std::optional<Run> best;
        for (const Run &r : runs) {
            if (!qualifiesFor(r.distanceMetres, target.metres))
                continue;
            const bool better = !best
                || r.durationSeconds < best->durationSeconds
                || (r.durationSeconds == best->durationSeconds && r.date < best->date);
            if (better)
                best = r;
        }
        if (best)
            result.append({target, *best});
    }
    return result;
}

} // namespace plainrun
