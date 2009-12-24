#ifndef MAINAAMODEL_H
#define MAINAAMODEL_H

#include <QtGui>
#include "mpiViewModelBase.h"
#include "cachedCalculations.h"

class aaRow: public baseRow
{
public:
    enum { row_Description, row_CostBasis, row_Value, row_ValueP, row_Gain, row_GainP, row_Target, row_Offset, row_Holdings };
    static const QStringList columns;
    static const QVariantList columnsType;

    aaRow(const QString &sort): baseRow(sort) {}

    QVariant columnType(int column) const { return columnsType.at(column); }
    static aaRow* getAARow(const calculations::portfolioDailyInfo *info, const cachedCalculations::dailyInfo &aaInfo,
        const globals::assetAllocation &aa, const QString &sort);
    static QMap<int, QString> fieldNames();
};

class mainAAModel: public mpiViewModelBase
{
public:

    mainAAModel(const QList<baseRow*> &rows, QList<int> viewableColumns, const calculations::portfolioDailyInfo *info, const double &aaThreshold, QTableView *parent = 0):
        mpiViewModelBase(rows, viewableColumns, parent), m_totalValue(info->totalValue), m_threshold(aaThreshold), m_costBasis(info->costBasis) { }

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    double m_totalValue;
    double m_threshold;
    double m_costBasis;
};

#endif // MAINAAMODEL_H