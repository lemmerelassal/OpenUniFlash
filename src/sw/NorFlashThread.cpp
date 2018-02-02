/*
OpenUniFlash - A universal NAND and NOR Flash programmer
Copyright (C) 2010-2018  Lemmer EL ASSAL, Axel GEMBE, Maël Blocteur

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
Also add information on how to contact you by electronic and paper mail.
*/

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
    progressRangeChanged(startAddr,endAddr);

    progress = startAddr;
    progressChanged(progress);

    for (uint32_t c = 0; c < norArgs.chipCount && !canceled; c++) {
        binfo.base = addr;
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
                    /*statusTextAdded*/progressSetText(tr("Reading address 0x%1 (%2)")
                                    .arg(addr, 0, 16)
                                    .arg(SizeSpinBox::formatSize(blockSize))
                                    );

                    QByteArray flashBuf(blockSize, 0xFF);
                    ::NOR_Read(flashBuf.data(), addr, blockSize);
                    ::RxStart();
                    if (fileBuf != flashBuf) {
                        //binfo.erased = flashBuf == erasedBuf;
                        binfo.do_erase = flashBuf != erasedBuf;
                        binfo.do_flash = fileBuf != erasedBuf;
                        binfo.file = fileBuf;
                        binfo.flash = flashBuf;
                        binfo.length = blockSize;
                        ret[addr] = binfo;
                    }

                    RxSpeedUpdate(::GetRxSpeed());
                    TxSpeedUpdate(::GetTxSpeed());
                } else {
                    //binfo.erased = false; /* We actually don't know */
                    binfo.do_erase = true;
                    binfo.do_flash = true;
                    binfo.file = fileBuf;
                    binfo.length = blockSize;
                    ret[addr] = binfo;
                }
                progress += norArgs.regions.region[r].blockSize;
                progressChanged(progress);
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

    progress=0;

    /* Correct the progresss range based on the sectors to be flashed */
    progressMax = progress + (blockMap.size() /* * 2 */);
    //if (commonArgs.verify) progressMax += blockMap.size();
    progressRangeChanged(0, progressMax);
    progressChanged(progress);

    /* Erase the blocks in the map */
    ibe = blockMap.end();


    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
        int addr = (*ib).first;
        uint32_t len = (*ib).second.length;
        uint32_t base = (*ib).second.base;
        QByteArray erasedBuf(len, 0xFF);
        QByteArray flashBuf(len, 0xFF);


        if ((*ib).second.do_erase) {
            progressSetText(tr("Erasing address 0x%1 (%2)")
                            .arg(addr, 0, 16)
                            .arg(SizeSpinBox::formatSize(len))
                            );



            /*statusTextAdded(tr("Status 0x%1")
                        .arg(*/ ::NOR_Erase(addr, base);/*, 0, 16)*/
//                        );



//            do {
//                ::NOR_Reset();
//                ::NOR_Read(flashBuf.data(), addr, len);
//                statusTextAdded(tr("Status 0x%1")
//                            .arg(::NOR_Erase(addr), 0, 16)
//                            );
//            } while(flashBuf != erasedBuf);


        }

        //progressChanged(++progress);
        progressChanged(++progress);
    }

    if (canceled) return;


    progress=0;

    /* Correct the progresss range based on the sectors to be flashed */
    progressMax = progress + (blockMap.size() /* * 2 */);
    //if (commonArgs.verify) progressMax += blockMap.size();
    progressRangeChanged(0, progressMax);
    progressChanged(progress);

    /* Flash the blocks in the map */
    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
        uint32_t addr = (*ib).first;
        uint32_t len = (*ib).second.length;
        uint32_t base = (*ib).second.base;
        uint8_t attempts = 0;
        bool success = false;
        QByteArray verifyBuf(len, 0xFF);
        progressSetText(tr("Flashing address 0x%1 (%2)...")
                        .arg(addr, 0, 16)
                        .arg(SizeSpinBox::formatSize(len))

                        );
        while((!success) && (attempts<64)) {
            attempts++;

            ::NOR_Program(base, addr, (*ib).second.file.data(), len, verifyBuf.data());
            RxStart();
            progressSetText(tr("Verifying address 0x%1 (%2)")
                            .arg(addr, 0, 16)
                            .arg(SizeSpinBox::formatSize(len))
                            );
            ::NOR_Read(verifyBuf.data(), addr, len);
            RxStart();
            RxSpeedUpdate(::GetRxSpeed());
            TxSpeedUpdate(::GetTxSpeed());
            if ((*ib).second.file == verifyBuf)
            {
                progressSetText(tr("Successfully flashed and verified after %1 attempts.").arg(attempts));
                success = true;
            }
        }
        if(!success)
            statusTextAdded(tr("Flashing address 0x%1 (%2) failed.")
                            .arg(addr, 0, 16)
                            .arg(SizeSpinBox::formatSize(len))

                            );
        progressChanged(++progress);
        progressChanged(++progress);

    }

//    if (canceled || !commonArgs.verify) return;

//    /* Verify the blocks in the map */
//    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
//        uint32_t addr = (*ib).first;
//        uint32_t len = (*ib).second.length;

//        statusTextAdded(tr("Verifying address 0x%1 (%2)")
//                        .arg(addr, 0, 16)
//                        .arg(SizeSpinBox::formatSize(len))
//                        );

//        ::NOR_Read(verifyBuf.data(), addr, len);
//        RxSpeedUpdate(::GetRxSpeed());
//        TxSpeedUpdate(::GetTxSpeed());

//        if ((*ib).second.file != verifyBuf) {
//            statusTextAdded(tr("Verification of address 0x%1 failed").arg(addr, 0, 16));
//            if (commonArgs.abortOnError)
//                break;
//        }

//        progressChanged(++progress);
//    }
}
