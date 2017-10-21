#pragma once

#include <map>

#include <QFile>

#include "NorCommonThread.h"

class NorFlashThread : public NorCommonThread
{
    Q_OBJECT

public:
    NorFlashThread(const QString& dumpFile, CommonArgs commonArgs, NorArgs norArgs);

protected:
    virtual void Operation();

private:
    struct BlockInfo {
        QByteArray file;
        QByteArray flash;
        uint32_t length;
        bool erased;
        bool do_erase;
        bool do_flash;
        uint32_t base;
    };

    typedef std::map<uint32_t, BlockInfo> BlockMap;
    BlockMap getBlockMap(QFile& file, int& progress);

    const QString flashFile;
};
