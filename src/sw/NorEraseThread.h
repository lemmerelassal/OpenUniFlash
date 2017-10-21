#pragma once

#include "NorCommonThread.h"

class NorEraseThread : public NorCommonThread
{
    Q_OBJECT

public:
    NorEraseThread(CommonArgs commonArgs, NorArgs norArgs);

protected:
    virtual void Operation();
};
