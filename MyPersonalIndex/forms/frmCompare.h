#ifndef FRMCOMPARE_H
#define FRMCOMPARE_H

#include <QtGui>
#include "frmCompare_UI.h"

class frmCompare : public QDialog
{
    Q_OBJECT

public:
    frmCompare();

private:
    frmCompare_UI ui;
};

#endif // FRMCOMPARE_H