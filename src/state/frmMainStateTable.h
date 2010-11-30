#ifndef FRMMAINSTATETABLE_H
#define FRMMAINSTATETABLE_H

#include "frmMainState.h"
#include "settings.h"

class QComboBox;
class mpiViewModelBase;
class frmMainTableView_UI;
class frmMainStateTable : public frmMainState
{
    Q_OBJECT

public:
    frmMainStateTable(const portfolio &portfolio_, const calculatorNAV &calculator_, const settings &settings_, QWidget *parent_);

    virtual ~frmMainStateTable();

    QWidget* mainWidget();

protected slots:
    void sortIndexChanged(int index_);
    void modifyColumns();
    void refreshTab();

protected:
    settings m_settings;
    frmMainTableView_UI *ui;

    virtual void setupUI();
    virtual settings::columns columnEnumValue() = 0;
    virtual QMap<int, QString> tableColumns() = 0;
    virtual frmMainTableView_UI* createUI();
    virtual mpiViewModelBase* createModel(int beginDate_, int endDate_) = 0;

    void setSortDropDown();
};

#endif // FRMMAINSTATETABLE_H