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

#include <algorithm>

#include <QFile>

#include "NandDumpThread.h"
#include "Flasher.h"

#include "utilities.h"

NandDumpThread::NandDumpThread(const QString& _dumpFile0, const QString& _dumpFile1,
                               const QString& _dumpFile2, const QString& _dumpFile3,
                               CommonArgs _commonArgs, DualNandArgs _dualNandArgs) :
    NandCommonThread(_commonArgs, _dualNandArgs), dumpFile0(_dumpFile0), dumpFile1(_dumpFile1), dumpFile2(_dumpFile2),
    dumpFile3(_dumpFile3)
{
}

void NandDumpThread::Operation()
{
//    dualNandArgs.nand[0].chipSelect = 0;
//    dualNandArgs.nand[1].chipSelect = 1;
//    /*if (dualNandArgs.nand[0].enabled && dualNandArgs.nand[1].enabled) {
//        DualNandOperation(dualNandArgs);
//    } else */if (dualNandArgs.nand[0].enabled) {
//        NandOperation(dualNandArgs.nand[0]);
//    } /*else*/ if (dualNandArgs.nand[1].enabled) {
//        NandOperation(dualNandArgs.nand[1]);
//    }

    dualNandArgs.Settings.bytesPerBlock = dualNandArgs.Settings.bytesPerPage * dualNandArgs.Settings.pageCount;
    NandOperation(dualNandArgs.nand[0]);

}


//important!!!
bool NandDumpThread::NandOperation(NandArgs nandArgs)
{
    QFile fp0, fp1, fp2, fp3;

    //    statusTextAdded(tr("Dumping NAND %1").arg(nandArgs.chipSelect + 1));
    QString StatusText("");
    StatusText=tr("Dumping NANDs (Selected:");
    for(int i=0; i<2; i++)
    {
        if(dualNandArgs.nand[i].enabled[0])
            StatusText.append(tr(" [NAND %1 Primary]").arg(i+1));
        if(dualNandArgs.nand[i].enabled[1])
            StatusText.append(tr(" [NAND %1 Secondary]").arg(i+1));
    }
    StatusText.append(tr(" )"));
    statusTextAdded(StatusText);



    int progressMax, progress, numBlocks;

    numBlocks = (dualNandArgs.range.end - dualNandArgs.range.start) + 1;

    /* One pass is always needed for read */
    progressMax = numBlocks;
    /* If we do verify, we need an additional pass */
    if (commonArgs.verify) progressMax += numBlocks;

    /* Initialize progress range, first guesstimate */
    progress = 0;
    progressRangeChanged(0, progressMax);
    progressChanged(0);



    if(dualNandArgs.nand[0].enabled[0])
    {
        fp0.setFileName(dumpFile0);
        fp0.open(QIODevice::WriteOnly);
    }


    if(dualNandArgs.nand[0].enabled[1])
    {
        fp1.setFileName(dumpFile1);
        fp1.open(QIODevice::WriteOnly);
    }


    if(dualNandArgs.nand[1].enabled[0])
    {
        fp2.setFileName(dumpFile2);
        fp2.open(QIODevice::WriteOnly);

    }

    if(dualNandArgs.nand[1].enabled[1])
    {
        fp3.setFileName(dumpFile3);
        fp3.open(QIODevice::WriteOnly);
    }



    QByteArray readBuf0(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray readBuf1(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray readBuf2(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray readBuf3(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf0(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf1(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf2(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf3(dualNandArgs.Settings.bytesPerBlock, 0xFF);

    char * buffer[4];







    ::NAND_Configure(dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.bigBlock, 0/*nandArgs.chipSelect*/, dualNandArgs.Settings.read_page_delay_us);
    uint32_t block = dualNandArgs.range.start;
    while (!canceled && block <= dualNandArgs.range.end)
    {
        /*statusTextAdded*/ progressSetText(tr("Dumping [Block %1/%2]").arg(block).arg(dualNandArgs.range.end));
        buffer[0] = dualNandArgs.nand[0].enabled[0]?readBuf0.data():0;
        buffer[1] = dualNandArgs.nand[0].enabled[1]?readBuf1.data():0;
        buffer[2] = dualNandArgs.nand[1].enabled[0]?readBuf2.data():0;
        buffer[3] = dualNandArgs.nand[1].enabled[1]?readBuf3.data():0;


        //::NAND_ReadBlock(block, readBuf.data(), nandArgs.chipSelect, nandArgs.bPrimary);
        ::NAND_ReadBlock(block, buffer, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.bigBlock);
        progressChanged(++progress);
        RxSpeedUpdate(::GetRxSpeed());
        TxSpeedUpdate(::GetTxSpeed());

        if (commonArgs.verify) {


            buffer[0] = dualNandArgs.nand[0].enabled[0]?verifyBuf0.data():0;
            buffer[1] = dualNandArgs.nand[0].enabled[1]?verifyBuf1.data():0;
            buffer[2] = dualNandArgs.nand[1].enabled[0]?verifyBuf2.data():0;
            buffer[3] = dualNandArgs.nand[1].enabled[1]?verifyBuf3.data():0;

            //::NAND_ReadBlock(block, verifyBuf.data(), nandArgs.chipSelect, nandArgs.bPrimary);
            ::NAND_ReadBlock(block, buffer, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.bigBlock);
            progressChanged(++progress);
            RxSpeedUpdate(::GetRxSpeed());
            TxSpeedUpdate(::GetTxSpeed());

            if (readBuf0 != verifyBuf0) {
                statusTextAdded(tr("Verification of block %1 on NAND A (Primary) failed").arg(block));
                if (commonArgs.abortOnError)
                    break;
            }


            if (readBuf1 != verifyBuf1) {
                statusTextAdded(tr("Verification of block %1 on NAND A (Secondary) failed").arg(block));
                if (commonArgs.abortOnError)
                    break;
            }


            if (readBuf2 != verifyBuf2) {
                statusTextAdded(tr("Verification of block %1 on NAND B (Primary) failed").arg(block));
                if (commonArgs.abortOnError)
                    break;
            }


            if (readBuf3 != verifyBuf3) {
                statusTextAdded(tr("Verification of block %1 on NAND B (Secondary) failed").arg(block));
                if (commonArgs.abortOnError)
                    break;
            }
        }

        if(dualNandArgs.nand[0].enabled[0])
        {
            fp0.write(readBuf0);
        }


        if(dualNandArgs.nand[0].enabled[1])
        {
            fp1.write(readBuf1);
        }


        if(dualNandArgs.nand[1].enabled[0])
        {
            fp2.write(readBuf2);

        }

        if(dualNandArgs.nand[1].enabled[1])
        {
            fp3.write(readBuf3);
        }

//        fp.write(readBuf);

        block++;
    }

    if(dualNandArgs.nand[0].enabled[0])
    {
        fp0.close();
    }


    if(dualNandArgs.nand[0].enabled[1])
    {
        fp1.close();
    }


    if(dualNandArgs.nand[1].enabled[0])
    {
        fp2.close();

    }

    if(dualNandArgs.nand[1].enabled[1])
    {
        fp3.close();
    }
//    fp.close();

    return true;
}

bool NandDumpThread::DualNandOperation(DualNandArgs dualNandArgs)
{

    //important!!!
//    statusTextAdded(tr("Dumping NAND 1 & 2"));

//    int progressMax, progress, numBlocks, numBlocks0, numBlocks1;

//    numBlocks0 = (dualNandArgs.range.end - dualNandArgs.range.start) + 1;
//    numBlocks1 = (dualNandArgs.range.end - dualNandArgs.range.start) + 1;
//    numBlocks = std::max(numBlocks0, numBlocks1);


//    /* One pass is always needed for read */
//    progressMax = numBlocks;
//    /* If we do verify, we need an additional pass */
//    if (commonArgs.verify) progressMax += numBlocks;

//    /* Initialize progress range, first guesstimate */
//    progress = 0;
//    progressRangeChanged(0, progressMax);
//    progressChanged(0);

//    QString fileName0 = getDualFileName(dumpFile, dualNandArgs.nand[0].chipSelect);
//    QString fileName1 = getDualFileName(dumpFile, dualNandArgs.nand[1].chipSelect);

//    QFile fp0(fileName0);
//    if (!fp0.open(QIODevice::WriteOnly)) {
//        statusTextAdded(tr("Failed to open file for writing"));
//        return false;
//    }

//    QFile fp1(fileName1);
//    if (!fp1.open(QIODevice::WriteOnly)) {
//        statusTextAdded(tr("Failed to open file for writing"));
//        fp0.close();
//        return false;
//    }

//    ::NAND_Configure(dualNandArgs.nand[0].bytesPerPage, 1<<dualNandArgs.nand[0].pageCount, dualNandArgs.nand[0].blockCount, dualNandArgs.nand[0].bigBlock, dualNandArgs.nand[0].chipSelect, dualNandArgs.nand[0].read_page_delay_us);
//    ::NAND_Configure(dualNandArgs.nand[1].bytesPerPage, 1<<dualNandArgs.nand[1].pageCount, dualNandArgs.nand[1].blockCount, dualNandArgs.nand[1].bigBlock, dualNandArgs.nand[1].chipSelect, dualNandArgs.nand[0].read_page_delay_us);

//    uint32_t block0 = dualNandArgs.nand[0].range.start;
//    uint32_t block1 = dualNandArgs.nand[1].range.start;
//    while (!canceled && progress < progressMax) {
//        statusTextAdded(tr("Dumping block %1@NAND 1 and %2@NAND 2").arg(block0).arg(block1));

//        QByteArray readBuf0(dualNandArgs.nand[0].bytesPerBlock, 0xFF);
//        QByteArray readBuf1(dualNandArgs.nand[1].bytesPerBlock, 0xFF);

//        /*if (block0 <= dualNandArgs.nand[0].range.end && block1 < dualNandArgs.nand[1].range.end) {
//            ::NAND_ReadDualBlock(block0, block1, readBuf0.data(), readBuf1.data());
//        } else */if (block0 <= dualNandArgs.nand[0].range.end) {
//            //::NAND_ReadBlock(block0, readBuf0.data(), dualNandArgs.nand[0].chipSelect, dualNandArgs.nand[0].bPrimary);
//        } /*else*/ if (block1 <= dualNandArgs.nand[1].range.end) {
//            //::NAND_ReadBlock(block1, readBuf1.data(), dualNandArgs.nand[1].chipSelect, dualNandArgs.nand[1].bPrimary);
//        }

//        progressChanged(++progress);

//        if (commonArgs.verify) {
//            QByteArray verifyBuf0(dualNandArgs.Settings.bytesPerBlock, 0xFF);
//            QByteArray verifyBuf1(dualNandArgs.Settings.bytesPerBlock, 0xFF);

//            /*if (block0 <= dualNandArgs.nand[0].range.end && block1 < dualNandArgs.nand[1].range.end) {
//                ::NAND_ReadDualBlock(block0, block1, verifyBuf0.data(), verifyBuf1.data());
//            } else */if (block0 <= dualNandArgs.nand[0].range.end) {
//                //::NAND_ReadBlock(block0, verifyBuf0.data(), dualNandArgs.nand[0].chipSelect, dualNandArgs.nand[0].bPrimary);
//            } /*else*/ if (block1 <= dualNandArgs.nand[1].range.end) {
//                //::NAND_ReadBlock(block1, verifyBuf1.data(), dualNandArgs.nand[1].chipSelect, dualNandArgs.nand[1].bPrimary);
//            }

//            progressChanged(++progress);

//            if (block0 <= dualNandArgs.nand[0].range.end && readBuf0 != verifyBuf0) {
//                statusTextAdded(tr("Verification of block %1@NAND 1 failed").arg(block0));
//                if (commonArgs.abortOnError)
//                    break;
//            }

//            if (block1 <= dualNandArgs.nand[1].range.end && readBuf1 != verifyBuf1) {
//                statusTextAdded(tr("Verification of block %1@NAND 2 failed").arg(block1));
//                if (commonArgs.abortOnError)
//                    break;
//            }
//        }

//        if (block0 <= dualNandArgs.range.end)
//            fp0.write(readBuf0);

//        if (block1 <= dualNandArgs.range.end)
//            fp1.write(readBuf1);

//        block0++;
//        block1++;
//    }

//    fp0.close();
//    fp1.close();

//    return true;
}

