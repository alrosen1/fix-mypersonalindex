#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtGui>
#include "queries.h"

class settings
{
public:
    typedef QMap<int, QList<int> > columns;
    enum { columns_Holdings, columns_AA, columns_Acct };

    int dataStartDate;
    bool splits;
    int version;
    bool tickersIncludeDividends;
    QVariant lastPortfolio;
    QSize windowSize;
    QPoint windowLocation;
    Qt::WindowState state;
    columns viewableColumns;

    settings(): version(0), lastPortfolio(QVariant()), state(Qt::WindowActive) {}
    void save();

    static void saveColumns(const int &columnsID, const QList<int> &columns);
    static settings loadSettings();

    bool operator==(const settings &other) const
    {
        // these are the only static properties, the other properties cannot be edited by the user
        return this->dataStartDate == other.dataStartDate
                && this->splits == other.splits;
    }

    bool operator!=(const settings &other) const
    {
        return !(*this == other);
    }

private:
    enum { getSettings_DataStartDate, getSettings_LastPortfolio, getSettings_WindowX, getSettings_WindowY, getSettings_WindowHeight,
           getSettings_WindowWidth, getSettings_WindowState, getSettings_Splits, getSettings_TickersIncludeDividends, getSettings_Version };
    static QString getSettings();

    enum { getSettingsColumns_ID, getSettingsColumns_ColumnID };
    static QString getSettingsColumns();

    static void loadSettingsInfo(settings &s, QSqlQuery *q);
    static void loadSettingsColumns(settings &s, QSqlQuery *q);
};

#endif // SETTINGS_H