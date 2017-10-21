#include "NandEraseThread.h"
#include "Flasher.h"

NandEraseThread::NandEraseThread(CommonArgs _commonArgs, DualNandArgs _dualNandArgs) :
    NandCommonThread(_commonArgs, _dualNandArgs)
{
}


void NandEraseThread::Operation()
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

//important
bool NandEraseThread::NandOperation(NandArgs nandArgs)
{
    QString text;
    bool do_erase[4] = {false,false,false,false};
    char * buffer[4];
    char status[4];
    QByteArray ffbuffer(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf0(dualNandArgs.Settings.bytesPerBlock, 0);
    QByteArray verifyBuf1(dualNandArgs.Settings.bytesPerBlock, 0);
    QByteArray verifyBuf2(dualNandArgs.Settings.bytesPerBlock, 0);
    QByteArray verifyBuf3(dualNandArgs.Settings.bytesPerBlock, 0);


    text = tr("Erasing NAND Flash(");

    if(do_erase[0])
        text.append(tr(" [NAND A Primary]"));
    if(do_erase[1])
        text.append(tr(" [NAND A Secondary]"));
    if(do_erase[2])
        text.append(tr(" [NAND B Primary]"));
    if(do_erase[3])
        text.append(tr(" [NAND B Secondary]"));

    statusTextAdded(text+" )");


    ::NAND_Configure(dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.bigBlock, 0/*nandArgs.chipSelect*/, dualNandArgs.Settings.read_page_delay_us);

    uint32_t block = dualNandArgs.range.start;
    for(int j=0; j<4; j++)
    {
        do_erase[j] = dualNandArgs.nand[(j/2)].enabled[(j%2)];
    }


    progressRangeChanged(block, dualNandArgs.range.end);
    uint32_t progress = block;

    while (!canceled && (block <= dualNandArgs.range.end))
    {
        text = tr("Erasing Block %1. (").arg(block);

        if(do_erase[0])
            text.append(tr(" [NAND A Primary]"));
        if(do_erase[1])
            text.append(tr(" [NAND A Secondary]"));
        if(do_erase[2])
            text.append(tr(" [NAND B Primary]"));
        if(do_erase[3])
            text.append(tr(" [NAND B Secondary]"));

        /*statusTextAdded*/progressSetText(text+" )");


        if(do_erase[0] || do_erase[1] || do_erase[2] || do_erase[3])
        {
            text = "";
            NAND_EraseBlock(block, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.blockCount, do_erase, status);

            RxStart();
            RxSpeedUpdate(::GetRxSpeed());
            TxSpeedUpdate(::GetTxSpeed());
            for(int j=0; j<4; j++)
            {
                if((status[j] & 1) && (do_erase[j]))
                {

                    //do_erase[j] = false;
                    statusTextAdded(tr("Bad block %1, status: 0x%2").arg(block).arg(status[j], 8, 16));
                }

                if(!(status[j] & 0x80) && (do_erase[j]))
                {

                    //do_erase[j] = false;
                    statusTextAdded(tr("Block %1 is protected, status: 0x%2").arg(block).arg(status[j], 8, 16));
                }
            }

            if(commonArgs.verify)
            {
                buffer[0] = (do_erase[0])?verifyBuf0.data():0;
                buffer[1] = (do_erase[1])?verifyBuf1.data():0;
                buffer[2] = (do_erase[2])?verifyBuf2.data():0;
                buffer[3] = (do_erase[3])?verifyBuf3.data():0;
                ::NAND_ReadBlock(block, buffer, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.bigBlock);

                if(do_erase[0])
                    if (ffbuffer  != verifyBuf0) {
                        statusTextAdded(tr("Verification of block %1 on NAND A (Primary) failed").arg(block));
                        if (commonArgs.abortOnError)
                            break;
                    }


                if(do_erase[1])
                    if (ffbuffer != verifyBuf1) {
                        statusTextAdded(tr("Verification of block %1 on NAND A (Secondary) failed").arg(block));
                        if (commonArgs.abortOnError)
                            break;
                    }


                if(do_erase[2])
                    if (ffbuffer != verifyBuf2) {
                        statusTextAdded(tr("Verification of block %1 on NAND B (Primary) failed").arg(block));
                        if (commonArgs.abortOnError)
                            break;
                    }


                if(do_erase[3])
                    if (ffbuffer != verifyBuf3) {
                        statusTextAdded(tr("Verification of block %1 on NAND B (Secondary) failed").arg(block));
                        if (commonArgs.abortOnError)
                            break;
                    }


            }
        }
        block++;
        progressChanged(progress++);
    }


//    statusTextAdded(tr("Erasing NAND %1").arg(nandArgs.chipSelect + 1));

//    int progressMax, progress, numBlocks;

//    numBlocks = (nandArgs.range.end - nandArgs.range.start) + 1;
//    nandArgs.bytesPerPage = nandArgs.bigBlock ? 2048 : 512;
//    if (nandArgs.raw) nandArgs.bytesPerPage += (nandArgs.bytesPerPage / 512) * 16;
//    nandArgs.bytesPerBlock = (1<<nandArgs.pageCount) * nandArgs.bytesPerPage;


//    /* One pass is always needed for erase */
//    progressMax = numBlocks;
//    /* If we do verify, we need an additional pass */
//    if (commonArgs.verify) progressMax += numBlocks;

//    /* Initialize progress range, first guesstimate */
//    progress = 0;
//    progressRangeChanged(0, progressMax);
//    progressChanged(0);
//    RxSpeedUpdate(::GetRxSpeed());
//    TxSpeedUpdate(::GetTxSpeed());

//    ::NAND_Configure(nandArgs.bytesPerPage, 1<<nandArgs.pageCount, nandArgs.blockCount, nandArgs.bigBlock, nandArgs.chipSelect, nandArgs.read_page_delay_us);

//    uint32_t block = nandArgs.range.start;
//    QByteArray erasedBlock(nandArgs.bytesPerBlock, 0xFF);
//    while (!canceled && block <= nandArgs.range.end) {
//        statusTextAdded(tr("Erasing block %1").arg(block));
//        ::NAND_EraseBlock(block, nandArgs.chipSelect, nandArgs.bPrimary);
//        progressChanged(++progress);
//        RxSpeedUpdate(::GetRxSpeed());
//        TxSpeedUpdate(::GetTxSpeed());

//        if (commonArgs.verify) {
//            QByteArray verifyBuf(nandArgs.bytesPerBlock, 0xFF);
//            //::NAND_ReadBlock(block, verifyBuf.data(), nandArgs.chipSelect, nandArgs.bPrimary);
//            progressChanged(++progress);
//            RxSpeedUpdate(::GetRxSpeed());
//            TxSpeedUpdate(::GetTxSpeed());

//            if (erasedBlock != verifyBuf) {
//                statusTextAdded(tr("Verification of block %1 failed").arg(block));
//                if (commonArgs.abortOnError)
//                    break;
//            }
//        }

//        block++;
//    }

//    return true;
}
