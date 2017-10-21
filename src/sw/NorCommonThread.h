#pragma once

#include "FlasherThread.h"

class NorCommonThread : public FlasherThread
{
    Q_OBJECT

public:
    NorCommonThread(CommonArgs commonArgs, NorArgs norArgs);

protected:
    NorArgs norArgs;
};
