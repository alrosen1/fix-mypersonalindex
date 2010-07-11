#include "portfolioFactory.h"



QMap<int, portfolio> portfolioFactory::getPortfolios(const queries &dataSource_)
{
#ifdef CLOCKTIME
    QTime t;
    t.start();
#endif

    loadPortfoliosInfo(dataSource_);
    loadPortfoliosAA(dataSource_);
    loadPortfoliosAcct(dataSource_);
    loadPortfoliosSecurity(dataSource_);
    loadPortfoliosSecurityAA(dataSource_);
    loadPortfoliosSecurityTrades(dataSource_);
    loadPortfoliosExecutedTrades(dataSource_);
    loadPortfoliosNAV(dataSource_);

#ifdef CLOCKTIME
    qDebug("Time elapsed: %d ms (portfolio)", t.elapsed());
#endif

    return m_portfolios;
}

void portfolioFactory::loadPortfoliosInfo(const queries &dataSource_)
{
    QSqlQuery q = dataSource_select(queries::table_Portfolios, queries::portfoliosColumns);
    while(q.next())
    {
        portfolio p(q.value(queries::portfoliosColumns_ID).toInt());

        p.attributes().description = q.value(queries::portfoliosColumns_Description).toString();
        p.attributes().startDate = q.value(queries::portfoliosColumns_StartDate).toInt();
        p.attributes().dividends = q.value(queries::portfoliosColumns_Dividends).toBool();
        p.attributes().costBasis = (costBasis)q.value(queries::portfoliosColumns_CostBasis).toInt();
        p.attributes().startValue = q.value(queries::portfoliosColumns_StartValue).toInt();
        p.attributes().aaThreshold = q.value(queries::portfoliosColumns_AAThreshold).toInt();
        p.attributes().aaThresholdMethod = (portfolioInfo::thesholdMethod)q.value(queries::portfoliosColumns_ThresholdMethod).toInt();
        p.attributes().holdingsShowHidden = q.value(queries::portfoliosColumns_HoldingsShowHidden).toBool();
        p.attributes().navSortDesc = q.value(queries::portfoliosColumns_NAVSortDesc).toBool();
        p.attributes().aaShowBlank = q.value(queries::portfoliosColumns_AAShowBlank).toBool();
        p.attributes().correlationShowHidden = q.value(queries::portfoliosColumns_CorrelationShowHidden).toBool();
        p.attributes().acctShowBlank = q.value(queries::portfoliosColumns_AcctShowBlank).toBool();
        p.attributes().holdingsSort = q.value(queries::portfoliosColumns_HoldingsSort).toString();
        p.attributes().aaSort = q.value(queries::portfoliosColumns_AASort).toString();
        p.attributes().acctSort = q.value(queries::portfoliosColumns_AcctSort).toString();

        m_portfolios.insert(p.attributes().id, p);
    }
}

void portfolioFactory::loadPortfoliosAA(const queries &dataSource_)
{
    QSqlQuery q = dataSource_select(queries::table_AA, queries::aaColumns);
    while(q.next())
    {
        assetAllocation aa(
            q.value(queries::aaColumns_ID).toInt(),
            q.value(queries::aaColumns_PortfolioID).toInt(),
            q.value(queries::aaColumns_Description).toString()
        );

        if (!q.value(queries::aaColumns_Target).isNull())
            aa.target = q.value(queries::aaColumns_Target).toDouble();

        m_portfolios[aa.parent].assetAllocations().insert(aa.id, aa);
    }
}

void portfolioFactory::loadPortfoliosAcct(const queries &dataSource_)
{
    QSqlQuery q = dataSource_.select(queries::table_Acct, queries::acctColumns);
    while(q.next())
    {
        account acct(
            q.value(queries::acctColumns_ID).toInt(),
            q.value(queries::acctColumns_PortfolioID).toInt(),
            q.value(queries::acctColumns_Description).toString()
        );

        if (!q.value(queries::acctColumns_TaxRate).isNull())
            acct.taxRate = q.value(queries::acctColumns_TaxRate).toDouble();
        acct.taxDeferred = q.value(queries::acctColumns_TaxDeferred).toBool();
        acct.costBasis = (costBasis)q.value(queries::acctColumns_CostBasis).toInt();

        m_portfolios[acct.parent].accounts().insert(acct.id, acct);
    }
}

void portfolioFactory::loadPortfoliosSecurity(const queries &dataSource_)
{
    QSqlQuery q = dataSource_.select(queries::table_Security, queries::SecurityColumns);
    while(q.next())
    {
        security sec(
            q.value(queries::securityColumns_ID).toInt(),
            q.value(queries::securityColumns_PortfolioID).toInt(),
            q.value(queries::securityColumns_Symbol).toString()
        );

        if (!q.value(queries::securityColumns_Account).isNull())
            sec.account = q.value(queries::securityColumns_Account).toInt();
        if (!q.value(queries::securityColumns_Expense).isNull())
            sec.expense = q.value(queries::securityColumns_Expense).toDouble();
        sec.divReinvest = q.value(queries::securityColumns_DivReinvest).toBool();
        sec.cashAccount = q.value(queries::securityColumns_CashAccount).toBool();
        sec.includeInCalc = q.value(queries::securityColumns_IncludeInCalc).toBool();
        sec.hide = q.value(queries::securityColumns_Hide).toBool();

        if(!sec.cashAccount)
            sec.setHistoricalPrices(priceFactory().getPrices(sec.description, dataSource_));

        m_portfolios[sec.parent].securities().insert(sec.id, sec);
    }
}

void portfolioFactory::loadPortfoliosSecurityAA(const queries &dataSource_)
{
    QSqlQuery q = dataSource_.select(queries::table_SecurityAA, queries::SecurityAAColumns, QString());
    while(q.next())
        m_portfolios[q.value(queries::securityAAColumns_Count).toInt()].securities()[q.value(queries::securityAAColumns_SecurityID).toInt()].aa.insert(
            q.value(queries::securityAAColumns_AAID).toInt(),
            q.value(queries::securityAAColumns_Percent).toDouble()
        );
}

void portfolioFactory::loadPortfoliosSecurityTrades(const queries &dataSource_)
{
    QSqlQuery q = dataSource_.select(queries::table_SecurityTrades, queries::SecurityTradeColumns, QString());
    while(q.next())
    {
        trade t(
               q.value(queries::securityTradeColumns_ID).toInt(),
               q.value(queries::securityTradeColumns_SecurityID).toInt()
        );

        t.type = (trade::tradeType)q.value(queries::securityTradeColumns_Type).toInt();
        t.value = q.value(queries::securityTradeColumns_Value).toDouble();
        if (!q.value(queries::securityTradeColumns_Price).isNull())
            t.price = q.value(queries::securityTradeColumns_Price).toDouble();
        t.commission = q.value(queries::securityTradeColumns_Commission).toDouble();
        if (!q.value(queries::securityTradeColumns_CashAccountID).isNull())
            t.cashAccount = q.value(queries::securityTradeColumns_CashAccountID).toInt();
        t.frequency = (tradeDateCalendar::frequency)q.value(queries::securityTradeColumns_Frequency).toInt();
        if (!q.value(queries::securityTradeColumns_Date).isNull())
            t.date = q.value(queries::securityTradeColumns_Date).toInt();
        if (!q.value(queries::securityTradeColumns_StartDate).isNull())
            t.startDate = q.value(queries::securityTradeColumns_StartDate).toInt();
        if (!q.value(queries::securityTradeColumns_EndDate).isNull())
            t.endDate = q.value(queries::securityTradeColumns_EndDate).toInt();

        m_portfolios[q.value(queries::securityTradeColumns_Count).toInt()].securities()[t.parent].trades.insert(t.id, t);
    }
}

void portfolioFactory::loadPortfoliosExecutedTrades(const queries &dataSource_)
{
    QSqlQuery q = dataSource_.select(queries::table_ExecutedTrades, queries::executedTradesColumns);

    while(q.next())
        m_portfolios[q.value(queries::executedTradesColumns_Count).toInt()]
            .securities()[q.value(queries::executedTradesColumns_SecurityID).toInt()]
            .executedTrades.insert
            (
                q.value(queries::executedTradesColumns_Date).toInt(),
                executedTrade(
                    q.value(queries::executedTradesColumns_Shares).toDouble(),
                    q.value(queries::executedTradesColumns_Price).toDouble(),
                    q.value(queries::executedTradesColumns_Commission).toDouble()
                )
            );
}

void portfolioFactory::loadPortfoliosNAV(const queries &dataSource_)
{
    QSqlQuery q = dataSource_.select(queries::table_NAV, queries::navColumns);
    while(q.next())
        m_portfolios[q.value(queries::navColumns_PortfolioID).toInt()].navHistory().insert(
            q.value(queries::navColumns_Date).toInt(),
            q.value(queries::navColumns_NAV).toDouble(),
            q.value(queries::navColumns_TotalValue).toDouble()
        );
}