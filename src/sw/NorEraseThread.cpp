#include "NorEraseThread.h"
#include "Flasher.h"
#include "utilities.h"

NorEraseThread::NorEraseThread(CommonArgs _commonArgs, NorArgs _norArgs) :
    NorCommonThread(_commonArgs, _norArgs)
{
}

void NorEraseThread::Operation()
{
    /* Initialize progress range */
    progressRangeChanged(norArgs.range.start, norArgs.range.end);
    progressChanged(norArgs.range.start);

    ::NOR_Configure(commonArgs, norArgs);
    ::NOR_Reset();

    uint32_t startAddr = norArgs.range.start;
    uint32_t endAddr = norArgs.range.end;
    uint32_t addr = 0;
    uint32_t base = 0;

    for (uint32_t c = 0; c < norArgs.chipCount && !canceled; c++) {
        base = addr;
        for (uint32_t r = 0; r < norArgs.regions.count && !canceled; r++) {
            for (uint32_t b = 0; b < norArgs.regions.region[r].blockCount && !canceled;
                 b++, addr += norArgs.regions.region[r].blockSize) {
                if (addr < startAddr)
                    continue;

                if (addr >= endAddr)
                    goto exit;

                uint32_t blockSize = norArgs.regions.region[r].blockSize;
                statusTextAdded(tr("Erasing address 0x%1 (0x%2 bytes)").arg(addr, 0, 16).arg(blockSize, 0, 16));
                ::NOR_Erase(addr, base);

                if (commonArgs.verify) {
                    QByteArray erasedSector(blockSize, 0xFF);
                    QByteArray verifyBuf(blockSize, 0x00);
                    ::NOR_Read(verifyBuf.data(), addr, blockSize);

                    if (erasedSector != verifyBuf) {
                        statusTextAdded(tr("Verification of address 0x%1 failed").arg(addr, 0, 16));
                        if (commonArgs.abortOnError)
                            break;
                    }
                }

                progressChanged(addr);
                RxSpeedUpdate(::GetRxSpeed());
                TxSpeedUpdate(::GetTxSpeed());
            }
        }
    }

exit:
    progressChanged(addr);
}
