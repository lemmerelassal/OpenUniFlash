#include <QFile>

#include "NandFlashThread.h"
#include "Flasher.h"


struct NandEraseBlockTask {
    uint32_t Block;
    bool enabled[4];
    struct NandEraseBlockTask *next;
};


NandFlashThread::NandFlashThread(const QString& _dumpFile0, const QString& _dumpFile1,
                                 const QString& _dumpFile2, const QString& _dumpFile3,
                                 CommonArgs _commonArgs, DualNandArgs _dualNandArgs) :
    NandCommonThread(_commonArgs, _dualNandArgs), dumpFile0(_dumpFile0), dumpFile1(_dumpFile1), dumpFile2(_dumpFile2),
    dumpFile3(_dumpFile3)
{
}


void NandFlashThread::Operation()
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
NandFlashThread::BlockMap NandFlashThread::getBlockMap(QFile& file, NandArgs nandArgs, int& progress)
{
//    BlockMap ret;
//    qint64 lastpos = file.pos();

//    uint32_t block = nandArgs.range.start;
//    while (!canceled && block <= nandArgs.range.end) {
//        QByteArray fileBuf = file.read(nandArgs.bytesPerBlock);

//        if (commonArgs.differential) {
//            statusTextAdded(tr("Reading block %1").arg(block));

//            QByteArray flashBuf(nandArgs.bytesPerBlock, 0xFF);
//            //::NAND_ReadBlock(block, flashBuf.data(), nandArgs.chipSelect, nandArgs.bPrimary);

//            if (fileBuf != flashBuf)
//                ret[block] = fileBuf;

//            progressChanged(++progress);
//            RxSpeedUpdate(::GetRxSpeed());
//            TxSpeedUpdate(::GetTxSpeed());
//        } else {
//            ret[block] = fileBuf;
//        }

//        block++;
//    }

//    file.seek(lastpos);

//    return ret;
}

//important
bool NandFlashThread::NandOperation(NandArgs nandArgs)
{
    QFile fp0, fp1, fp2, fp3;
    QString text;
    bool do_erase[4] = {false,false,false,false};
    bool do_flash[4] = {false,false,false,false};
    char status[4];

    QString StatusText("");
    StatusText=tr("Flashing NANDs (Selected:");
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
    //if (commonArgs.verify) progressMax += numBlocks;

    /* Initialize progress range, first guesstimate */
    progress = 0;
    progressRangeChanged(0, progressMax);
    progressChanged(0);



    if(dualNandArgs.nand[0].enabled[0])
    {
        fp0.setFileName(dumpFile0);
        fp0.open(QIODevice::ReadOnly);
    }


    if(dualNandArgs.nand[0].enabled[1])
    {
        fp1.setFileName(dumpFile1);
        fp1.open(QIODevice::ReadOnly);
    }


    if(dualNandArgs.nand[1].enabled[0])
    {
        fp2.setFileName(dumpFile2);
        fp2.open(QIODevice::ReadOnly);

    }

    if(dualNandArgs.nand[1].enabled[1])
    {
        fp3.setFileName(dumpFile3);
        fp3.open(QIODevice::ReadOnly);
    }

    QByteArray ffbuffer(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray readBuf0(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray readBuf1(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray readBuf2(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray readBuf3(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray writeBuf0(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray writeBuf1(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray writeBuf2(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray writeBuf3(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf0(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf1(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf2(dualNandArgs.Settings.bytesPerBlock, 0xFF);
    QByteArray verifyBuf3(dualNandArgs.Settings.bytesPerBlock, 0xFF);

    char * buffer[4];







    ::NAND_Configure(dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.bigBlock, 0/*nandArgs.chipSelect*/, dualNandArgs.Settings.read_page_delay_us);
    uint32_t block = dualNandArgs.range.start;
    while (!canceled && block <= dualNandArgs.range.end)
    {
        for(int i=0; i<4; i++)
        {
            do_erase[i] = false;
            do_flash[i] = false;
        }

        if(dualNandArgs.nand[0].enabled[0])
        {
            writeBuf0 = fp0.read(dualNandArgs.Settings.bytesPerBlock);
        }


        if(dualNandArgs.nand[0].enabled[1])
        {
            writeBuf1 = fp1.read(dualNandArgs.Settings.bytesPerBlock);
        }


        if(dualNandArgs.nand[1].enabled[0])
        {
            writeBuf2 = fp2.read(dualNandArgs.Settings.bytesPerBlock);

        }

        if(dualNandArgs.nand[1].enabled[1])
        {
            writeBuf3 = fp3.read(dualNandArgs.Settings.bytesPerBlock);
        }





        if(commonArgs.differential)
        {
            progressSetText(tr("Dumping block %1").arg(block));
            buffer[0] = dualNandArgs.nand[0].enabled[0]?readBuf0.data():0;
            buffer[1] = dualNandArgs.nand[0].enabled[1]?readBuf1.data():0;
            buffer[2] = dualNandArgs.nand[1].enabled[0]?readBuf2.data():0;
            buffer[3] = dualNandArgs.nand[1].enabled[1]?readBuf3.data():0;
            ::NAND_ReadBlock(block, buffer, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.bigBlock);



            if(dualNandArgs.nand[0].enabled[0])
                if(writeBuf0 != readBuf0)
                {
                    do_flash[0] = (writeBuf0 != ffbuffer);
                    do_erase[0] = (readBuf0 != ffbuffer);
                }

            if(dualNandArgs.nand[0].enabled[1])
                if(writeBuf1 != readBuf1)
                {
                    do_flash[1] = (writeBuf1 != ffbuffer);
                    do_erase[1] = (readBuf1 != ffbuffer);
                }

            if(dualNandArgs.nand[1].enabled[0])
                if(writeBuf2 != readBuf2)
                {
                    do_flash[2] = (writeBuf2 != ffbuffer);
                    do_erase[2] = (readBuf2 != ffbuffer);
                }

            if(dualNandArgs.nand[1].enabled[1])
                if(writeBuf3 != readBuf3)
                {
                    do_flash[3] = (writeBuf3 != ffbuffer);
                    do_erase[3] = (readBuf3 != ffbuffer);
                }
        } else {
            for(int j=0; j<4; j++)
            {
                do_erase[j] = dualNandArgs.nand[(j/2)].enabled[(j%2)];
                do_flash[j] = dualNandArgs.nand[(j/2)].enabled[(j%2)];
            }
        }

//        text = tr("Erasing Block %1. (").arg(block);

//            if(do_erase[0])
//                text.append(tr(" [NAND A Primary]"));
//            if(do_erase[1])
//                text.append(tr(" [NAND A Secondary]"));
//            if(do_erase[2])
//                text.append(tr(" [NAND B Primary]"));
//            if(do_erase[3])
//                text.append(tr(" [NAND B Secondary]"));



//        if(do_erase[0] || do_erase[1] || do_erase[2] || do_erase[3])
//        {
//            progressSetText(text+" )");
//            NAND_EraseBlock(block, dualNandArgs.Settings.bytesPerBlock, dualNandArgs.Settings.blockCount, do_erase, status);
//            RxStart();
//            RxSpeedUpdate(::GetRxSpeed());
//            TxSpeedUpdate(::GetTxSpeed());
//            for(int j=0; j<4; j++)
//            {
//                if((status[j] & 1) && (do_erase[j]))
//                {
////                    do_flash[j] = false;
////                    do_erase[j] = false;
//                    statusTextAdded(tr("Bad block %1, status: 0x%2").arg(block).arg(QString::number((unsigned char)status[j],16)));
//                }

//                if(!(status[j] & 0x80) && (do_erase[j]))
//                {
////                    do_flash[j] = false;
////                    do_erase[j] = false;
//                    statusTextAdded(tr("Block %1 is protected, status: 0x%2").arg(block).arg(QString::number((unsigned char)status[j],16)));
//                }
//            }
//        }



        text = tr("Flashing Block %1. (").arg(block);



        if(do_flash[0])
            text.append(tr(" [NAND A Primary]"));
        if(do_flash[1])
            text.append(tr(" [NAND A Secondary]"));
        if(do_flash[2])
            text.append(tr(" [NAND B Primary]"));
        if(do_flash[3])
            text.append(tr(" [NAND B Secondary]"));

        buffer[0] = do_flash[0]?writeBuf0.data():0;
        buffer[1] = do_flash[1]?writeBuf1.data():0;
        buffer[2] = do_flash[2]?writeBuf2.data():0;
        buffer[3] = do_flash[3]?writeBuf3.data():0;




        if(do_flash[0] || do_flash[1] || do_flash[2] || do_flash[3])
        {
            progressSetText(text+" )");


//            NAND_FlashBlock(block, do_erase, status);

            NAND_ProgramBlock(block, buffer, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.bigBlock, status );
            RxStart();

            RxSpeedUpdate(::GetRxSpeed());
            TxSpeedUpdate(::GetTxSpeed());

            for(int j=0; j<4; j++)
            {
                if((status[j] & 1) && (do_flash[j]))
                {
                    statusTextAdded(tr("Bad block %1, status: 0x%2").arg(block).arg(QString::number((unsigned char)status[j],16)));
                }

                if(!(status[j] & 0x80) && (do_flash[j]))
                {
                    statusTextAdded(tr("Block %1 is protected, status: 0x%2").arg(block).arg(QString::number((unsigned char)status[j],16)));
                }
            }

        }



        if (commonArgs.verify) {

            progressSetText(tr("Verifying block %1").arg(block));

            buffer[0] = (do_erase[0]||do_flash[0])?verifyBuf0.data():0;
            buffer[1] = (do_erase[1]||do_flash[1])?verifyBuf1.data():0;
            buffer[2] = (do_erase[2]||do_flash[2])?verifyBuf2.data():0;
            buffer[3] = (do_erase[3]||do_flash[3])?verifyBuf3.data():0;

            ::NAND_ReadBlock(block, buffer, dualNandArgs.Settings.blockCount, dualNandArgs.Settings.pageCount, dualNandArgs.Settings.bytesPerPage, dualNandArgs.Settings.bigBlock);

            RxSpeedUpdate(::GetRxSpeed());
            TxSpeedUpdate(::GetTxSpeed());



            if(do_erase[0] || do_flash[0])
                if (writeBuf0 != verifyBuf0) {
                    statusTextAdded(tr("Verification of block %1 on NAND A (Primary) failed").arg(block));
                    if (commonArgs.abortOnError)
                        break;
                }


            if(do_erase[1] || do_flash[1])
                if (writeBuf1 != verifyBuf1) {
                    statusTextAdded(tr("Verification of block %1 on NAND A (Secondary) failed").arg(block));
                    if (commonArgs.abortOnError)
                        break;
                }


            if(do_erase[2] || do_flash[2])
                if (writeBuf2 != verifyBuf2) {
                    statusTextAdded(tr("Verification of block %1 on NAND B (Primary) failed").arg(block));
                    if (commonArgs.abortOnError)
                        break;
                }


            if(do_erase[3] || do_flash[3])
                if (writeBuf3 != verifyBuf3) {
                    statusTextAdded(tr("Verification of block %1 on NAND B (Secondary) failed").arg(block));
                    if (commonArgs.abortOnError)
                        break;
                }
        }



//        fp.write(readBuf);

        progressChanged(++progress);
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






//    statusTextAdded(tr("Flashing NAND %1").arg(nandArgs.chipSelect + 1));

//    int progressMax, progress, numBlocks;
//    BlockMap blockMap;
//    BlockMap::iterator ib, ibe;

//    numBlocks = (nandArgs.range.end - nandArgs.range.start) + 1;
//    nandArgs.bytesPerPage = nandArgs.bigBlock ? 2048 : 512;
//    if (nandArgs.raw) nandArgs.bytesPerPage += (nandArgs.bytesPerPage / 512) * 16;
//    nandArgs.bytesPerBlock = (1<<nandArgs.pageCount) * nandArgs.bytesPerPage;

//    /* Two passes are always needed for erase and program */
//    progressMax = numBlocks * 2;
//    /* If we do differential, we need an additional pass to read the flash */
//    if (commonArgs.differential) progressMax += numBlocks;
//    /* If we do verify, we need an additional pass to read the flash, too */
//    if (commonArgs.verify) progressMax += numBlocks;

//    /* Initialize progress range, first guesstimate */
//    progress = 0;
//    progressRangeChanged(0, progressMax);
//    progressChanged(0);

//    QString fileName = getDualFileName(flashFile, nandArgs.chipSelect);

//    QFile fp(flashFile); //fileName);
//    if (!fp.open(QIODevice::ReadOnly)) {
//        statusTextAdded(tr("Failed to open file for reading"));
//        return false;
//    }

//    ::NAND_Configure(nandArgs.bytesPerPage, 1<<nandArgs.pageCount, nandArgs.blockCount, nandArgs.bigBlock, nandArgs.chipSelect, nandArgs.read_page_delay_us);

//    /* Determine the blocks to be flashed. If differential is not selected, all blocks are in the list */
//    blockMap = getBlockMap(fp, nandArgs, progress);
//    if (canceled) goto exit;

//    /* Check if there is anything to flash */
//    if (blockMap.size() == 0) {
//        statusTextAdded(tr("Nothing to flash!"));
//        progressRangeChanged(0, numBlocks);
//        progressChanged(numBlocks);
//        goto exit;
//    }

//    /* Correct the progresss range based on the sectors to be flashed */
//    progressMax = numBlocks + blockMap.size();
//    if (commonArgs.differential) progressMax += blockMap.size();
//    if (commonArgs.verify) progressMax += blockMap.size();
//    progressRangeChanged(0, progressMax);
//    progressChanged(progress);

//    /* Erase the blocks in the map */
//    ibe = blockMap.end();
//    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
//        int block = (*ib).first;
//        statusTextAdded(tr("Erasing block %1").arg(block));
//        if (::NAND_EraseBlock(block, nandArgs.chipSelect, nandArgs.bPrimary) & 0x1) {
//            statusTextAdded(tr("Found bad block %1").arg(block));
//            if (commonArgs.abortOnError)
//                goto exit;
//        }
//        progressChanged(++progress);
//        RxSpeedUpdate(::GetRxSpeed());
//        TxSpeedUpdate(::GetTxSpeed());
//    }

//    if (canceled) goto exit;

//    /* Flash the blocks in the map */
//    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
//        int block = (*ib).first;
//        statusTextAdded(tr("Flashing block %1").arg(block));
//        if (::NAND_ProgramBlock(block, (*ib).second.data(), nandArgs.chipSelect, nandArgs.bPrimary) & 0x1) {
//            statusTextAdded(tr("Found bad block %1").arg(block));
//            if (commonArgs.abortOnError)
//                goto exit;
//        }

//        progressChanged(++progress);
//        RxSpeedUpdate(::GetRxSpeed());
//        TxSpeedUpdate(::GetTxSpeed());
//    }

//    if (canceled || !commonArgs.verify) goto exit;

//    /* Verify the blocks in the map */
//    for (ib = blockMap.begin(); ib != ibe && !canceled; ib++) {
//        int block = (*ib).first;
//        statusTextAdded(tr("Verifying block %1").arg(block));

//        QByteArray verifyBuf(nandArgs.bytesPerBlock, 0xFF);
//        //::NAND_ReadBlock(block, verifyBuf.data(), nandArgs.chipSelect, nandArgs.bPrimary);

//        if ((*ib).second != verifyBuf) {
//            statusTextAdded(tr("Verification of block %1 failed").arg(block));
//            if (commonArgs.abortOnError)
//                break;
//        }

//        progressChanged(++progress);
//        RxSpeedUpdate(::GetRxSpeed());
//        TxSpeedUpdate(::GetTxSpeed());
//    }

//exit:
//    fp.close();
//    return true;
}
