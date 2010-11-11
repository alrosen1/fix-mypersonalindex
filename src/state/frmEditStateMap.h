#ifndef FRMEDITSTATEMAP_H
#define FRMEDITSTATEMAP_H

#include "frmEditState.h"

template<class T>
class QList;
class objectKey;
class frmEditStateMap : public frmEditState
{
public:
    frmEditStateMap(portfolio portfolio_, QObject *parent_);
    virtual ~frmEditStateMap() {}

protected:
    virtual void validationError(objectKey* key_, const QString &errorMessage_) = 0;

    template <class T>
    bool validateMap(QMap<int, T> &map_);

    template <class T>
    QList<objectKey*> mapToList(QMap<int, T> &map_) const;
};

#endif // FRMEDITSTATEMAP_H
