#include "frmMainCorrelation_State.h"
#include "mainCorrelationModel.h"
#include "frmMainTableViewTree_UI.h"
#include "historicalNAV.h"
#include "calculatorCorrelation.h"

frmMainCorrelation_State::frmMainCorrelation_State(int portfolioID_, const QMap<int, portfolio> &portfolios_, const settings &settings_,
        const QHash<QString, historicalPrices> &prices_, QWidget *parent_):
    frmMainStateTableWithTree(portfolioID_, portfolios_, settings_, prices_, parent_)
{
    setupUI(true);
    connect(treeWidget(), SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(itemChecked(QTreeWidgetItem*,int)));
}

frmMainCorrelation_State::~frmMainCorrelation_State()
{
}

settings::columns frmMainCorrelation_State::columnEnumValue()
{
    return settings::columns_Dummy;
}

QMap<int, QString> frmMainCorrelation_State::tableColumns()
{
    return QMap<int, QString>();
}

void frmMainCorrelation_State::itemChecked(QTreeWidgetItem *item_, int /*column_*/)
{
    if (!item_->parent())
        return;

    int beginDate = ui->toolbarDateBeginEdit->date().toJulianDay();
    int endDate = ui->toolbarDateEndEdit->date().toJulianDay();
    treeItemKey key = createKeyFromTreeItem(item_);

    if (item_->checkState(0) == Qt::Unchecked)
    {
        m_selectedItems.remove(key);
        static_cast<mainCorrelationModel*>(ui->table->model())->remove(correlationRow(key.portfolioID, key.type, key.id, key.displayText));
        return;
    }

    QMap<correlationRow, double> correlations;
    historicalNAV nav = calculateNAV(key, beginDate, endDate);

    foreach(const treeItemKey &item, m_selectedItems)
    {
        if (item == key)
            continue;

        double correlation = calculatorCorrelation::correlation(nav, calculateNAV(item, beginDate, endDate));
        correlations.insert(correlationRow(item.portfolioID, item.type, item.id, item.displayText), correlation);
    }

    m_selectedItems.insert(key);
    static_cast<mainCorrelationModel*>(ui->table->model())->add(
        new correlationRow(key.portfolioID, key.type, key.id, key.displayText, correlations),
        correlationRow(key.portfolioID, key.type, key.id, key.displayText)
    );
}

mpiViewModelBase* frmMainCorrelation_State::createModel(int beginDate_, int endDate_)
{
    if (m_selectedItems.isEmpty())
        return new mainCorrelationModel(QList<baseRow*>(), ui->table);

    QList<treeItemKey> items = m_selectedItems.toList();
    QHash<treeItemKey, correlationRow*> rowsMap;
    QList<baseRow*> rows;

    foreach(const treeItemKey &item, items)
    {
        correlationRow *row = new correlationRow(item.portfolioID, item.type, item.id, item.displayText, QMap<correlationRow, double>());
        rows.append(row);
        rowsMap.insert(item, row);
    }

    for(int i = 0; i < items.count() - 1; ++i)  // once we reach count - 1, all combinations will already be calculated
    {
        treeItemKey item1 = items.at(i);

        for (int x = i + 1; x < items.count(); ++x)
        {
            treeItemKey item2 = items.at(x);

            double correlation = calculatorCorrelation::correlation(calculateNAV(item1, beginDate_, endDate_), calculateNAV(item2, beginDate_, endDate_));
            rowsMap[item1]->correlationValues.insert(correlationRow(item2.portfolioID, item2.type, item2.id, item2.displayText), correlation);
            rowsMap[item2]->correlationValues.insert(correlationRow(item1.portfolioID, item1.type, item1.id, item1.displayText), correlation);
        }
    }

    return new mainCorrelationModel(rows, ui->table);
}
