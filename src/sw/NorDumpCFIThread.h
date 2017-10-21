#pragma once

#include <QFile>

#include "NorCommonThread.h"

class NorDumpCFIThread : public NorCommonThread
{
    Q_OBJECT

public:
    NorDumpCFIThread(const QString& dumpFile, CommonArgs commonArgs, NorArgs *norArgs);

protected:
    virtual void Operation();

private:
    const QString dumpFile;
    NorArgs *norargs;
    void Decode(const QByteArray& cfi);
};
