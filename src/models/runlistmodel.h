#pragma once

#include "core/run.h"

#include <QAbstractListModel>
#include <QList>
#include <QLocale>

namespace plainrun {

// Read-only list of runs for the history view. Display strings are prepared
// here so QML stays declarative.
class RunListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        DateIsoRole,
        DateTextRole,
        DistanceTextRole,
        DurationTextRole,
        PaceTextRole,
        NoteRole,
    };

    explicit RunListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRuns(const QList<Run> &runs, const QDate &today);
    const QList<Run> &runs() const { return m_runs; }
    int count() const { return static_cast<int>(m_runs.size()); }

    Q_INVOKABLE int indexOfId(qint64 id) const;
    Q_INVOKABLE qint64 idAt(int row) const;

    // "Sat 27 Sep"; includes the year when it differs from the current year.
    static QString shortDate(const QDate &date, const QDate &today, const QLocale &locale = QLocale());

signals:
    void countChanged();

private:
    QList<Run> m_runs;
    QDate m_today;
    QLocale m_locale;
};

} // namespace plainrun
