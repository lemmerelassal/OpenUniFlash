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

#include "async_io.h"

#include <QThread>


Async_IO::Async_IO(usb_dev_handle *_handle) :
    canceled(false), handle(_handle), rxactive(false), txactive(false), TxSize(0), RxSize(0)
{
    /* Make the thread delete itself when it is finished */
    QObject::connect(
        this, SIGNAL(finished()),
        this, SLOT(deleteLater())
        );

    txbuffer = (uint8_t *) malloc(16384);
    rxbuffer = (uint8_t *) malloc(16384);
}

void Async_IO::onCanceled()
{
    canceled = true;
}

void Async_IO::Operation()
{
    int res = 0;

    while(!canceled)
    {
        while(rxactive);
        rxactive = true;
        res = usb_bulk_read(handle, EP_IN, (char*) rxbuffer + RxSize, 16384-RxSize, 5000);
        if(res>0)
            RxSize += res;
        rxactive = false;
        while(txactive);
        txactive = true;
        res = usb_bulk_write(handle, EP_OUT, (char*) txbuffer, TxSize, 5000);
        if(res>0)
            TxSize -= res;
        txactive = false;
        Sleep(1000);
    }

}

bool Async_IO::FifoWrite(uint16_t length, uint8_t *data)
{
    while(txactive);
    if(length+TxSize > 16384)
        return false;
    txactive = true;
    memcpy(txbuffer+TxSize, data, length);
    TxSize += length;
    txactive = false;
}

bool Async_IO::FifoRead(uint16_t *length, uint8_t *data)
{
    while(rxactive);
    if(!RxSize)
        return false;
    rxactive = true;
    memcpy(data, rxbuffer, RxSize);
    *length = RxSize;
    RxSize = 0;
    rxactive = false;
    return true;
}
