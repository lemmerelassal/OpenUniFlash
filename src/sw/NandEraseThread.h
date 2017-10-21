#pragma once

#include "NandCommonThread.h"

class NandEraseThread : public NandCommonThread
{
    Q_OBJECT

public:
    NandEraseThread(CommonArgs commonArgs, DualNandArgs nandArgs);

private:
    virtual bool NandOperation(NandArgs nandArgs);
    void Operation();
};
