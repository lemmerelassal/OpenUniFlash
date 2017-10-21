#pragma once

#include <map>

#include <QFile>

#include "NandCommonThread.h"

class NandFlashThread : public NandCommonThread
{
    Q_OBJECT

public:
    NandFlashThread(const QString& _dumpFile0, const QString& _dumpFile1,
                                     const QString& _dumpFile2, const QString& _dumpFile3,
                                     CommonArgs _commonArgs, DualNandArgs _dualNandArgs);

private:
    typedef std::map<int, QByteArray> BlockMap;
    BlockMap getBlockMap(QFile& file, NandArgs nandArgs, int& progress);

    virtual bool NandOperation(NandArgs nandArgs);
    void Operation();

    const QString dumpFile0;
    const QString dumpFile1;
    const QString dumpFile2;
    const QString dumpFile3;
};
