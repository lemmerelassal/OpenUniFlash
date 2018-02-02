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

#include "FlasherThread.h"
#include "Flasher.h"

FlasherThread::FlasherThread(CommonArgs _commonArgs) :
    canceled(false), commonArgs(_commonArgs)
{
    /* Make the thread delete itself when it is finished */
    QObject::connect(
        this, SIGNAL(finished()),
        this, SLOT(deleteLater())
        );
}

void FlasherThread::run()
{
    if (!::CreateDevice()) {
        statusTextAdded(tr("Device not found!"));
        return;
    }

    uint16_t ver;
    ::GetVersion(&ver);
    ::RxStart();
    if(BuildVersion == ver)
        Operation();
    else
        statusTextAdded(tr("ProgSkeet is not up to date. Version expected: %1.%2; Version used: %3.%4")
                        .arg(BuildVersion&0xFF, 8, 10, QChar('0')).toUpper()
                        .arg((BuildVersion>>8)&0xFF, 8, 10, QChar('0')).toUpper()
                        .arg(ver&0xFF, 8, 10, QChar('0')).toUpper()
                        .arg((ver>>8)&0xFF, 8, 10, QChar('0')).toUpper()
                        );

    ::RemoveDevice();

    statusTextAdded(tr("Finished"));
    RxSpeedUpdate(::GetRxSpeed());
    TxSpeedUpdate(::GetTxSpeed());
}

void FlasherThread::onCanceled()
{
    ::CancelOperations();
    canceled = true;
}
