#pragma once

#include "core/run.h"

#include <QList>
#include <QString>

namespace plainrun {

struct PbTarget {
    QString label;
    double metres = 0;
};

// Recognised distances: 1 km, 5 km, 10 km, half marathon, marathon.
const QList<PbTarget> &pbTargets();

// A run counts towards a distance when its recorded distance is at least 99%
// and at most 103% of that distance. GPS-measured races usually read slightly
// long (runners rarely follow the exact racing line) and only occasionally a
// little short, hence the asymmetric window. A 5.4 km run is not a 5 km PB.
inline constexpr double PbLowerTolerance = 0.99;
inline constexpr double PbUpperTolerance = 1.03;

bool qualifiesFor(qint64 distanceMetres, double targetMetres);

struct PersonalBest {
    PbTarget target;
    Run run;
};

// Fastest recorded time for each recognised distance that has a qualifying
// run. The recorded time is used as-is (no extrapolation). Ties go to the
// earlier run.
QList<PersonalBest> personalBests(const QList<Run> &runs);

} // namespace plainrun
