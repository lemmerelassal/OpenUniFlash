#pragma once

#include "NandCommonThread.h"

class NandDumpThread : public NandCommonThread
{
    Q_OBJECT

public:
    NandDumpThread(const QString& _dumpFile0, const QString& _dumpFile1,
                                   const QString& _dumpFile2, const QString& _dumpFile3,
                                   CommonArgs _commonArgs, DualNandArgs _dualNandArgs);

private:
    virtual void Operation();
    virtual bool NandOperation(NandArgs nandArgs);
    bool DualNandOperation(DualNandArgs dualNandArgs);

    const QString dumpFile0;
    const QString dumpFile1;
    const QString dumpFile2;
    const QString dumpFile3;
};
