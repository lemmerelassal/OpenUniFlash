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
