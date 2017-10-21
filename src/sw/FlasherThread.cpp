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
