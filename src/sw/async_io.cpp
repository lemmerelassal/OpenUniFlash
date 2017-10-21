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
