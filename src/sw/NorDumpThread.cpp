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

#include "NorDumpThread.h"
#include "Flasher.h"

NorDumpThread::NorDumpThread(const QString& _dumpFile, CommonArgs _commonArgs, NorArgs _norArgs) :
    NorCommonThread(_commonArgs, _norArgs), dumpFile(_dumpFile)
{
}

void NorDumpThread::Operation()
{
    uint32_t addr;
    uint32_t blockSize = 1 << 17; /* 128 KiB read block */
    QString unstablelines;

    /* Initialize progress range */
    progressRangeChanged(0, norArgs.range.end - norArgs.range.start);
    progressChanged(0);

    QFile fp(dumpFile);
    if (!fp.open(QIODevice::WriteOnly)) {
        statusTextAdded(tr("Failed to open file for writing"));
        return;
    }

    if (norArgs.range.sync)
        fp.seek(norArgs.range.start);

    int res = 0;

//    while(res < 0)
//    {
//        ::RemoveDevice();
//        ::CreateDevice();
        res = ::NOR_Configure(commonArgs, norArgs);
//    }

//    if(res < 0)
//    {
//        ::RemoveDevice();
//        ::CreateDevice();
//        res = ::NOR_Reset();
//    }

    for (addr = norArgs.range.start; addr < norArgs.range.end && !canceled; addr += blockSize) {
        /*statusTextAdded*/progressSetText(tr("Dumping [Sector %1/%2][0x%3]").arg((addr/blockSize)+1).arg(norArgs.range.end/blockSize).arg(addr, 0, 16));

        QByteArray readBuf(blockSize, 0xFF);
        res = ::NOR_Read(readBuf.data(), addr, blockSize);

//        while(res < 0)
//        {
//            ::RemoveDevice();
//            ::CreateDevice();
//            res = ::NOR_Read(readBuf.data(), addr, blockSize);
//        }


        if (commonArgs.verify) {
            QByteArray verifyBuf(blockSize, 0xFF);


            res = ::NOR_Read(verifyBuf.data(), addr, blockSize);
            RxStart();
//            while(res < 0)
//            {
//                ::RemoveDevice();
//                ::CreateDevice();
//                ::NOR_Read(verifyBuf.data(), addr, blockSize);
//            }

            if (readBuf != verifyBuf) {
 //               for(int i=0; i<blockSize; i++)
//                    if(*(verifyBuf.data()+i) != *(readBuf.data()+i))
//                    {
//                        uint16_t fault = *(verifyBuf.data()+i) ^ *(readBuf.data()+i);
//                        unstablelines = tr("Unstable");
//                        for(int j=0; j<16; j++)
//                            if(fault & (1 << j))
//                                unstablelines.append("D%i").arg(j);
//                    }
                statusTextAdded(tr("Verification of address 0x%1 failed").arg(addr, 0, 16));

                if (commonArgs.abortOnError)
                    break;
            }
        }
        RxStart();
        fp.write(readBuf);

        progressChanged(addr);
        RxSpeedUpdate(::GetRxSpeed());
        TxSpeedUpdate(::GetTxSpeed());
    }

    progressChanged(addr);

    fp.close();
}
