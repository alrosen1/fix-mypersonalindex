#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include <qmath.h>
#include <qnumeric.h>
#include <QHash>
#include <QMap>
#include "snapshot.h"
#include "portfolio.h"
#include "splits.h"
#include "epsilon.h"
#include "avgPricePerShare.h"
#include "tradeDateCalendar.h"

#ifdef CLOCKTIME
#include <QTime>
#endif

class calculations
{
public:
    calculations(const portfolio &portfolio_):
        m_portfolio(portfolio_)
    {}

    snapshotPortfolio portfolioSnapshot(int date_, bool calcAveragePrices_ = false);
    snapshotSecurity securitySnapshot(int date_, int id_);
    snapshot assetAllocationSnapshot(int date_, int id_);
    snapshot accountSnapshot(int date_, int id_);

    navInfoStatistic changeOverTime(const objectKey &key_, int beginDate_, int endDate_, bool dividends_);
    navInfoStatistic changeOverTime(int beginDate_, int endDate_); // overload that uses portfolio's NAV values, much faster than recalc'ing

    static double correlation(const navInfoStatistic &first_, const navInfoStatistic &second_);
    static double change(double beginValue, double endValue_, double activity_, double dividends_, double beginNav_ = 1);

private:
    const portfolio m_portfolio;
    QHash<int, snapshotPortfolio> m_cache;

    snapshot snapshotByKey(int date_, const objectKey &key_);
    navInfoStatistic changeOverTime(const QString &symbol_, int beginDate_, int endDate_, bool dividends_);
    QMap<int, double> avgPricePerShare(int date_);
};


#endif // CALCULATIONS_H
