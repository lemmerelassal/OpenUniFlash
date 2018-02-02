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

#include "NandCommonThread.h"
#include "Flasher.h"

#include "utilities.h"

NandCommonThread::NandCommonThread(CommonArgs _commonArgs, DualNandArgs _dualNandArgs) :
    FlasherThread(_commonArgs), dualNandArgs(_dualNandArgs)
{
}
//important
void NandCommonThread::Operation()
{
//    std::list<NandArgs>::iterator ina, inae;
//    size_t i;
//    int chipSelect;

//    for (i = 0, chipSelect = 0; i < COUNTOF(dualNandArgs.nand) && !canceled; i++, chipSelect++) {

//        if (!dualNandArgs.nand[i].enabled)
//            continue;


//        if(dualNandArgs.nand[i].CE_A)
//        {
//            initArgs(dualNandArgs.nand[i], chipSelect, 1);
//            if (!NandOperation(dualNandArgs.nand[i]) && commonArgs.abortOnError)
//                break;
//        }

//        if(dualNandArgs.nand[i].CE_B)
//        {
//            initArgs(dualNandArgs.nand[i], chipSelect, 0);

//            if (!NandOperation(dualNandArgs.nand[i]) && commonArgs.abortOnError)
//                break;
//        }

//    }
}

QString NandCommonThread::getDualFileName(const QString& fileName, const int chipSelect)
{
//    return (chipSelect == 0) ? fileName : QString("%1.%2").arg(fileName).arg(chipSelect);
}


//obsolete
void NandCommonThread::initArgs(NandArgs& nandArgs, const int chipSelect, const int bPrimary)
{
//    nandArgs.bytesPerPage = nandArgs.bigBlock ? 2048 : 512;
//    if (nandArgs.raw) nandArgs.bytesPerPage += (nandArgs.bytesPerPage / 512) * 16;

//    nandArgs.bytesPerBlock = nandArgs.pageCount * nandArgs.bytesPerPage;

//    nandArgs.chipSelect = chipSelect;
//    nandArgs.bPrimary = bPrimary;
}
