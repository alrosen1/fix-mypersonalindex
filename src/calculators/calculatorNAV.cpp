#include "calculatorNAV.h"
#include <qmath.h>
#include <qnumeric.h>
#include "portfolio.h"
#include "security.h"
#include "account.h"
#include "splits.h"
#include "snapshot.h"
#include "symbol.h"
#include "historicalNAV.h"
#include "functions.h"
#include "tradeDateCalendar.h"
#include "executedTrade.h"
#include "assetAllocationTarget.h"

snapshotSecurity calculatorNAV::securitySnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_)
{
    // check today's cache
    snapshotSecurity value = m_securitiesCache.value(date_).value(id_);
    if (!value.isNull())
        return value;

    // check if it needs to be calculated
    security s = portfolio_->securities().value(id_);
    if (!s.includeInCalc() || s.executedTrades().isEmpty())
        return snapshotSecurity(date_);

    // check if prior day is cached
    value = m_securitiesCache.value(
                priorDate_ == 0 ?
                    tradeDateCalendar::previousTradeDate(date_) :
                    priorDate_
            ).value(id_);

    splits splitRatio(s.splits(), date_, value.date);

    if (value.date != 0)
        // if there is a split between the cached date (exclusive) and today (inclusive), multiply all existing cached shares by those splits
        value.shares = value.shares * splitRatio.ratio(value.date);

    // start loop depending on cached date
    for(QMap<int, executedTrade>::const_iterator i = s.executedTrades().lowerBound(value.date + 1); i != s.executedTrades().constEnd(); ++i)
    {
       if (i.key() > date_)
           break;

       value.shares += i->shares * splitRatio.ratio(i.key());
       value.costBasis += (i->shares * i->price) + i->commission;
    }

    value.shares = functions::massage(value.shares); // zero out if needed
    value.date = date_;
    value.dividendAmount = value.shares * s.dividend(date_);
    if (s.dividendNAVAdjustment())
        value.dividendAmountNAV = value.dividendAmount;
    value.totalValue = value.shares * s.price(date_);
    value.expenseRatio = s.expenseRatio();

    account acct = portfolio_->accounts().value(s.account());
    value.setTaxLiability(acct.taxRate(), acct.taxDeferred());

    m_securitiesCache[date_].insert(id_, value);
    return value;
}

snapshot calculatorNAV::portfolioSnapshot(const portfolio *portfolio_, int date_, int priorDate_)
{
    snapshot value(date_);

    foreach(const security &s, portfolio_->securities())
        value.add(securitySnapshot(portfolio_, date_, s.id(), priorDate_));

    return value;
}

snapshot calculatorNAV::assetAllocationSnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_)
{
    snapshot value(date_);

    foreach(const security &s, portfolio_->securities())
        if (s.targets().contains(id_))
            value.add(securitySnapshot(portfolio_, date_, s.id(), priorDate_), s.targets().value(id_));

    return value;
}

snapshot calculatorNAV::accountSnapshot(const portfolio *portfolio_, int date_, int id_, int priorDate_)
{
    snapshot value(date_);

    foreach(const security &s, portfolio_->securities())
        if (id_ == s.account())
            value.add(securitySnapshot(portfolio_, date_, s.id(), priorDate_));

    return value;
}

snapshot calculatorNAV::symbolSnapshot(int date_, const symbol &key_, int beginDate_)
{
    snapshot value(date_);
    splits splitRatio(key_.splits(), date_, beginDate_);

    value.count = 1;
    value.costBasis = key_.price(beginDate_);
    value.dividendAmount = key_.dividend(date_) * splitRatio.ratio(beginDate_);
    if (key_.includeDividends())
        value.dividendAmountNAV = value.dividendAmount;
    value.totalValue = key_.price(date_) * splitRatio.ratio(beginDate_);

    return value;
}

snapshot calculatorNAV::snapshotByKey(const portfolio *portfolio_, int date_, const objectKeyBase &key_, int beginDate_, int priorDate_)
{
    switch(key_.type())
    {
        case objectType_AA:
            return assetAllocationSnapshot(portfolio_, date_, key_.id(), priorDate_);
        case objectType_Account:
            return accountSnapshot(portfolio_, date_, key_.id(), priorDate_);
        case objectType_Portfolio:
            return portfolioSnapshot(portfolio_, date_, priorDate_);
        case objectType_Security:
            return securitySnapshot(portfolio_, date_, key_.id(), priorDate_);
        case objectType_Symbol:
            return symbolSnapshot(date_, static_cast<const symbol&>(key_), beginDate_);
        case objectType_Trade:
            // not implemented yet, a little too granular...
            return snapshot(0);
    }
    return snapshot(0);
}

int calculatorNAV::beginDateByKey(const portfolio *portfolio_, const objectKeyBase &key_)
{
    switch(key_.type())
    {
        case objectType_AA:
        case objectType_Account:
        case objectType_Portfolio:
        case objectType_Security:
            return portfolio_->startDate();
        case objectType_Symbol:
            return static_cast<const symbol&>(key_).beginDate();
        case objectType_Trade:
            // not implemented yet, a little too granular...
            return 0;
    }
    return 0;
}

int calculatorNAV::endDateByKey(const portfolio *portfolio_, const objectKeyBase &key_)
{
    switch(key_.type())
    {
        case objectType_AA:
        case objectType_Account:
        case objectType_Portfolio:
            return portfolio_->endDate();
        case objectType_Security:
            return static_cast<const security&>(key_).endDate();
        case objectType_Symbol:
            return static_cast<const symbol&>(key_).endDate();
        case objectType_Trade:
            // not implemented yet, a little too granular...
            return 0;
    }
    return 0;
}

double calculatorNAV::nav(const portfolio *portfolio_, const objectKeyBase &key_, int beginDate_, int endDate_)
{
    return changeOverTime(portfolio_, key_, beginDate_, endDate_).nav(endDate_);
}

historicalNAV calculatorNAV::changeOverTime(const portfolio *portfolio_, const objectKeyBase &key_, int beginDate_, int endDate_)
{
    double navValue = 1;
    historicalNAV navHistory;

    beginDate_ = qMax(beginDateByKey(portfolio_, key_), beginDate_);
    endDate_ = qMin(endDateByKey(portfolio_, key_), endDate_);

    tradeDateCalendar calendar(beginDate_);
    if (beginDate_ > endDate_ || calendar.date() > endDate_)
        return navHistory;

    beginDate_ = calendar.date();
    snapshot priorSnapshot = snapshotByKey(portfolio_, beginDate_, key_, beginDate_, 0);
    navHistory.insert(beginDate_, navValue, priorSnapshot.totalValue, priorSnapshot.dividendAmount); // baseline nav

    foreach(int date, ++calendar)
    {
        if (date > endDate_)
            break;

        snapshot currentSnapshot = snapshotByKey(portfolio_, date, key_, beginDate_, priorSnapshot.date);

        navValue =
                    change(
                        priorSnapshot.totalValue,
                        currentSnapshot.totalValue,
                        currentSnapshot.costBasis - priorSnapshot.costBasis,
                        currentSnapshot.dividendAmountNAV,
                        navValue
                    );

        navHistory.insert(date, navValue, currentSnapshot.totalValue, currentSnapshot.dividendAmount);
        priorSnapshot = currentSnapshot;
    }

    // take last day's values
    navHistory.costBasis = priorSnapshot.costBasis;
    navHistory.expenseRatio = priorSnapshot.expenseRatio;
    navHistory.taxLiability = priorSnapshot.taxLiability;

    return navHistory;
}

double calculatorNAV::change(double beginValue_, double endValue_, double activity_, double dividends_, double beginNAV_)
{
    if (functions::isZero(endValue_))
        return beginNAV_;

    double nav;
    activity_ -= dividends_;
    if (activity_ < 0)
        nav = (endValue_ - activity_) / (beginValue_ / beginNAV_);
    else
        nav = endValue_ / ((beginValue_ + activity_) / beginNAV_);

    return (isnan(nav) || isinf(nav)) ? beginNAV_ : nav;
}
