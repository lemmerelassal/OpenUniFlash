#include "NorFlashThread.h"
#include "Flasher.h"
#include "FlashUtilities.h"
#include "sizespinbox.h"

NorFlashThread::NorFlashThread(const QString& _flashFile, CommonArgs _commonArgs, NorArgs _norArgs) :
    NorCommonThread(_commonArgs, _norArgs), flashFile(_flashFile)
{
}

NorFlashThread::BlockMap NorFlashThread::getBlockMap(QFile& file, int& progress)
{
    BlockInfo binfo;
    BlockMap ret;
    qint64 lastpos = file.pos();
    file.seek(0ll);

    uint32_t startAddr = norArgs.range.start;
    uint32_t endAddr = norArgs.range.end;
    uint32_t addr = 0;
    bool do_erase = false;

    for (uint32_t c = 0; c < norArgs.chipCount && !canceled; c++) {
        for (uint32_t r = 0; r < norArgs.regions.count && !canceled; r++) {
            QByteArray erasedBuf(norArgs.regions.region[r].blockSize, 0xFF);
            for (uint32_t b = 0; b < norArgs.regions.region[r].blockCount && !canceled;
                 b++, addr += norArgs.regions.region[r].blockSize) {
                if (addr < startAddr)
                    continue;

                if (addr >= endAddr)
                    goto exit;

                if (norArgs.range.sync)
                    file.seek(addr);

                uint32_t blockSize = norArgs.regions.region[r].blockSize;
                QByteArray fileBuf = file.read(blockSize);

                if (commonArgs.differential) {
                    statusTextAdded(tr("Reading address 0x%1 (%2)")
                                    .arg(addr, 0, 16)
                                    .arg(SizeSpinBox::formatSize(blockSize))
                                    );

                    QByteArray flashBuf(blockSize, 0xFF);
                    ::NOR_Read(flashBuf.data(), addr, blockSize);
                    do_erase = false;
                    if (fileBuf != flashBuf) {
                        do_erase = flashBuf == erasedBuf;
                        for(int j=0; j<blockSize; j++)
                            for(int i=0; i<8; i++)
                                if( (*(flashBuf.data()+j) & (1 << i)) < (*(fileBuf.data()+j) & (1 << i)) ) {
                                        do_erase = true;
                                        break;
                                }

                        binfo.erased = do_erase;//?false:(flashBuf == erasedBuf);
                        binfo.file = fileBuf;
                        binfo.flash = flashBuf;
                        binfo.length = blockSize;
                        ret[addr] = binfo;
                    }

                    progressChanged(++progress);
                } else {
                    binfo.erased = false; /* We actually don't know */
                    binfo.file = fileBuf;
                    binfo.length = blockSize;
                    ret[addr] = binfo;
                }
            }
        }
    }

exit:
    file.seek(lastpos);

    return ret;
}

void NorFlashThread::Operation()
{
    int progressMax, progress;
    BlockMap blockMap;
    BlockMap::iterator ib, ibe;
    uint32_t numBlocks = 0;

    for (uint32_t r = 0; r < norArgs.regions.count; r++)
        numBlocks += norArgs.regions.region[r].blockCount;
    numBlocks *= norArgs.chipCount;

    /* Two passes are always needed for erase and program */
    progressMax = numBlocks * 2;
    /* If we do differential, we need an additional pass to read the flash */
    if (commonArgs.differential) progressMax += numBlocks;
    /* If we do verify, we need an additional pass to read the flash, too */
    if (commonArgs.verify) progressMax += numBlocks;

    /* Initialize progress range, first guesstimate */
    progress = 0;
    progressRangeChanged(0, progressMax);
    progressChanged(0);

    QFile fp(flashFile);
    if (!fp.open(QIODevice::ReadOnly)) {
        statusTextAdded(tr("Failed to open file for reading"));
        return;
    }

    ::NOR_Configure(commonArgs, norArgs);

    ::NOR_Reset();

    /* Determine the sectors to be flashed. If differential is not selected, all sectors are in the list */
    blockMap = getBlockMap(fp, progress);
    fp.close();

    if (canceled) return;

    /* Check if there is anything to flash */
    if (blockMap.size() == 0) {
        statusTextAdded(tr("Nothing to flash!"));
        progressRangeChanged(0, numBlocks);
        progressChanged(numBlocks);
        return;
    }

    /* Correct the progresss range based on the sectors to be flashed */
    progressMax = progress + (blockMap.size() * 2);
    if (commonArgs.verify) progressMax += blockMap.size();
    progressRangeChanged(0, progressMax);
    progressChanged(progress);

    /* Erase the blocks in the map */
    ibe = blockMap.end();
    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
        int addr = (*ib).first;
        uint32_t len = (*ib).second.length;

        if (!(*ib).second.erased) {
            statusTextAdded(tr("Erasing address 0x%1 (%2)")
                            .arg(addr, 0, 16)
                            .arg(SizeSpinBox::formatSize(len))
                            );
            ::NOR_Erase(addr);
        }

        progressChanged(++progress);
    }

    if (canceled) return;

    /* Flash the blocks in the map */
    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
        uint32_t addr = (*ib).first;
        uint32_t len = (*ib).second.length;
        statusTextAdded(tr("Flashing address 0x%1 (%2)")
                        .arg(addr, 0, 16)
                        .arg(SizeSpinBox::formatSize(len))
                        );
        ::NOR_Program(addr, (*ib).second.file.data(), len);
        progressChanged(++progress);
    }

    if (canceled || !commonArgs.verify) return;

    /* Verify the blocks in the map */
    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
        uint32_t addr = (*ib).first;
        uint32_t len = (*ib).second.length;

        statusTextAdded(tr("Verifying address 0x%1 (%2)")
                        .arg(addr, 0, 16)
                        .arg(SizeSpinBox::formatSize(len))
                        );

        QByteArray verifyBuf(len, 0xFF);
        ::NOR_Read(verifyBuf.data(), addr, len);

        if ((*ib).second.file != verifyBuf) {
            statusTextAdded(tr("Verification of address 0x%1 failed").arg(addr, 0, 16));
            if (commonArgs.abortOnError)
                break;
        }

        progressChanged(++progress);
    }
}
