#pragma once

#include <QFile>

#include "NorCommonThread.h"

class NorDumpThread : public NorCommonThread
{
    Q_OBJECT

public:
    NorDumpThread(const QString& dumpFile, CommonArgs commonArgs, NorArgs norArgs);

protected:
    virtual void Operation();

private:
    const QString dumpFile;
};
