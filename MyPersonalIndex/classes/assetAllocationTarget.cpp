#include "assetAllocationTarget.h"

void assetAllocationTarget::insert(int id_, double target_)
{
    m_targets.insert(id_, target_);
    if (m_batchInProgress)
        m_valuesToBeInserted.append(id_);
}

void assetAllocationTarget::insertBatch(queries dataSource_)
{
    if (!this->hasParent())
        return;

    dataSource_.bulkInsert(queries::table_PortfolioSecurityAA, queries::portfolioSecurityAAColumns, *this);
    m_valuesToBeInserted.clear();
    queriesBatch::insertBatch();
}

void assetAllocationTarget::remove(const queries &dataSource_)
{
    m_targets.clear();
    if (this->hasParent())
        dataSource_.deleteSecurityItems(queries::table_PortfolioSecurityAA, this->parent);
}

QVariant assetAllocationTarget::data(int row_, int column_) const
{
    switch(column_)
    {
        case queries::portfolioSecurityAAColumns_AAID:
            return m_valuesToBeInserted.at(row_);
            break;
        case queries::portfolioSecurityAAColumns_Percent:
            return m_targets.value(m_valuesToBeInserted.at(row_));
            break;
        case queries::portfolioSecurityAAColumns_SecurityID:
            return this->parent;
            break;
    }
    return QVariant();
}
