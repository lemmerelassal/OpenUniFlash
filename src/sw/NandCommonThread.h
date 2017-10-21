#pragma once

#include <list>

#include "FlasherThread.h"

class NandCommonThread : public FlasherThread
{
    Q_OBJECT

public:
    NandCommonThread(CommonArgs commonArgs, DualNandArgs dualNandArgs);

protected:
    virtual void Operation();

    DualNandArgs dualNandArgs;

    QString getDualFileName(const QString& fileName, const int chipSelect);

    void initArgs(NandArgs& nandArgs, const int chipSelect, const int bPrimary);

private:
    virtual bool NandOperation(NandArgs nandArgs) = 0;
};
