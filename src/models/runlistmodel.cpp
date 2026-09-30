#include "models/runlistmodel.h"

#include "core/runformat.h"

namespace plainrun {

RunListModel::RunListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RunListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant RunListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= count())
        return {};
    const Run &r = m_runs.at(index.row());
    switch (role) {
    case IdRole:
        return r.id;
    case DateIsoRole:
        return formatIsoDate(r.date);
    case Qt::DisplayRole:
    case DateTextRole:
        return shortDate(r.date, m_today, m_locale);
    case DistanceTextRole:
        return formatKm(r.distanceMetres, 2, m_locale) + QStringLiteral(" km");
    case DurationTextRole:
        return formatDuration(r.durationSeconds);
    case PaceTextRole:
        return formatPace(paceSecondsPerKm(r.distanceMetres, r.durationSeconds)) + QStringLiteral("/km");
    case NoteRole:
        return r.note;
    default:
        return {};
    }
}

QHash<int, QByteArray> RunListModel::roleNames() const
{
    return {
        {IdRole, "runId"},
        {DateIsoRole, "dateIso"},
        {DateTextRole, "dateText"},
        {DistanceTextRole, "distanceText"},
        {DurationTextRole, "durationText"},
        {PaceTextRole, "paceText"},
        {NoteRole, "note"},
    };
}

void RunListModel::setRuns(const QList<Run> &runs, const QDate &today)
{
    const int oldCount = count();
    beginResetModel();
    m_runs = runs;
    m_today = today;
    m_locale = QLocale();
    endResetModel();
    if (oldCount != count())
        emit countChanged();
}

int RunListModel::indexOfId(qint64 id) const
{
    for (qsizetype i = 0; i < m_runs.size(); ++i) {
        if (m_runs.at(i).id == id)
            return static_cast<int>(i);
    }
    return -1;
}

qint64 RunListModel::idAt(int row) const
{
    if (row < 0 || row >= count())
        return -1;
    return m_runs.at(row).id;
}

QString RunListModel::shortDate(const QDate &date, const QDate &today, const QLocale &locale)
{
    if (today.isValid() && date.year() == today.year())
        return locale.toString(date, QStringLiteral("ddd d MMM"));
    return locale.toString(date, QStringLiteral("d MMM yyyy"));
}

} // namespace plainrun
