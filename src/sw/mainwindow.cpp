#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextStream>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sizespinbox.h"

#include "Flasher.h"
#include "FlashUtilities.h"

#include "NorDumpThread.h"
#include "NorFlashThread.h"
#include "NorEraseThread.h"
#include "NorDumpCFIThread.h"

#include "NandDumpThread.h"
#include "NandFlashThread.h"
#include "NandEraseThread.h"

#define FLASH_FILE_FILTER QApplication::translate("Global", "Binary files (*.bin);;All files (*)")
#define PATCH_FILE_FILTER QApplication::translate("Global", "Patch files (*.txt);;All files (*)")
#define CFI_FILE_FILTER QApplication::translate("Global", "CFI files (*.cfi);;All files (*)")

MainWindow* g_pMainWindow = NULL;

bool warnedresistornand = true;
bool warnedresistornor = true;

struct nand_manufacturers nand_manuf_ids[] = {
        {NAND_MFR_TOSHIBA, "Toshiba"},
        {NAND_MFR_SAMSUNG, "Samsung"},
        {NAND_MFR_FUJITSU, "Fujitsu"},
        {NAND_MFR_NATIONAL, "National"},
        {NAND_MFR_RENESAS, "Renesas"},
        {NAND_MFR_STMICRO, "ST Micro"},
        {NAND_MFR_HYNIX, "Hynix"},
        {NAND_MFR_MICRON, "Micron"},
        {NAND_MFR_AMD, "AMD"},
        {0x0, "Unknown"}
};

void DebugOut(const char* string)
{
    if (g_pMainWindow == NULL)
        return;

    //QMetaObject::invokeMethod(g_pMainWindow, "addStatusText", Qt::QueuedConnection, Q_ARG(QString, string));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), settings("ProgSkeet", "ProgSkeet")
{






//    warnedresistornand = false;
//    warnedresistornor = false;
    ui->setupUi(this);

    g_pMainWindow = this;
    ::DebugSetup(&DebugOut);

    setWindowTitle(TARGETNAME);

    memset(&commonArgs, 0, sizeof(commonArgs));
    memset(&norArgs, 0, sizeof(norArgs));
    memset(&dualNandArgs, 0, sizeof(dualNandArgs));

    QString text;

    for (int i = 0; i < 20; i++) {
        int j = 1 << i;

        text = QString::number(j);

        ui->cbNandBlockCount->addItem(text);
        ui->cbNandPagesPerBlock->addItem(text);

//        ui->cbNand2BlockCount->addItem(text);
//        ui->cbNand2PagesPerBlock->addItem(text);
    }

    for(int i=0; i<5;i++)
    {
        ui->cbNandPageSize->addItem(QString::number(512*(1<<i)));
        ui->cbNandAddressCycles->addItem(QString::number(i+3));
    }


    //ui->inputReadDelay->setText("50");

//    for(int i = 0; i < 16; i++)
//        ui->cbPeriod->addItem(tr("%1ns").arg( (i+1) * 20.8333333333333333));

    ui->spNorRangeStart->setEraseBlockRegions(&norArgs.regions);
    ui->spNorRangeEnd->setEraseBlockRegions(&norArgs.regions);
    ui->spNorRangeEnd->setExtraBlock(true);

    updateEraseBlockRegions();

    /* Select common tab at startup */
    ui->tabWidget->setCurrentIndex(0);

    presetsInit();

    NorPresetLoad();


    QFile file( "./changelog.txt");
    if( file.open( QFile::ReadOnly | QFile::Text ) )
    {
        QTextStream in(&file);

        ui->plainTextEdit->setPlainText(in.readAll());
    }
    file.close();


    QTextCursor cursor(ui->plainTextEdit->textCursor());
    cursor.movePosition(QTextCursor::Start);
    ui->plainTextEdit->setTextCursor(cursor);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_cbNorWriteMethod_currentIndexChanged(int index)
{
    norArgs.writeMethod = (NorWriteMethod)index;

    ui->spNorBufferedMaxBytes->setEnabled((NorWriteMethod)index == NorWriteMethod_BufferedWrite);
}


void MainWindow::on_cbNorWaitMethod_currentIndexChanged(int index)
{
    norArgs.waitMethod = (NorWaitMethod)index;
}

void MainWindow::on_cboxByteSwap_toggled(bool checked)
{
    commonArgs.byteSwap = checked;
}

void MainWindow::on_cboxVerify_toggled(bool checked)
{
    commonArgs.verify = checked;
}

void MainWindow::on_cboxDifferential_toggled(bool checked)
{
    commonArgs.differential = checked;
}

void MainWindow::on_cboxAbortOnError_clicked(bool checked)
{
    commonArgs.abortOnError = checked;
}

void MainWindow::on_btnNorDump_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Dump destination..."), 0, FLASH_FILE_FILTER);
    if(filename == "")
        return;

    disableButtons(true);
    NorDumpThread* ndt = new NorDumpThread(filename, commonArgs, norArgs);
    connectFlasherThread(ndt);
    ndt->start();
}

void MainWindow::on_btnNorFlash_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Flash source..."), 0, FLASH_FILE_FILTER);
    if(filename == "")
        return;

    disableButtons(true);
    NorFlashThread* nft = new NorFlashThread(filename, commonArgs, norArgs);
    connectFlasherThread(nft);
    nft->start();
}

uint16_t logbin(uint32_t value)
{
    uint16_t temp = 0;
    while(value >>= 1)
        temp++;
    return temp;
}

void MainWindow::on_btnNorDumpCFI_clicked()
{
    QString filename("");

    if(QMessageBox::question(this, tr("Dump CFI").arg(TARGETNAME), tr("Would you like to save CFI?"),
                             QMessageBox::Yes, QMessageBox::No)
            == QMessageBox::Yes)
         filename = QFileDialog::getSaveFileName(this, tr("CFI dump destination..."), "./cfi/", CFI_FILE_FILTER);
//    if(filename == "")
//        return;

    disableButtons(true);
    NorDumpCFIThread* ndt = new NorDumpCFIThread(filename, commonArgs, &norArgs);
    connectFlasherThread(ndt);
    ndt->start();

    while(!ndt->isFinished());

    if(filename != "") {
        NorPresetLoad();
        ui->cbNorPresets->setCurrentIndex(-1);
    }




        //norArgs.writeMethod = NorWaitMethod_RdyTrigger;



}

void MainWindow::on_btnNorErase_clicked()
{

    if(QMessageBox::critical(this, tr("WARNING").arg(TARGETNAME), tr("YOU ARE ABOUT TO ERASE THE NOR CHIP.\nTHIS IS IRREVERSIBLE."),
                             QMessageBox::Ok, QMessageBox::Cancel)
            == QMessageBox::Cancel)
       return;

    disableButtons(true);
    NorEraseThread* net = new NorEraseThread(commonArgs, norArgs);
    connectFlasherThread(net);
    net->start();
}

void MainWindow::on_btnNorDetect_clicked()
{
    if (!::CreateDevice()) {
        QMessageBox::critical(this, tr("%1 read id information").arg(TARGETNAME),
                              tr("Device not found!."));
        return;
    }

    uint16_t ver;
    ::GetVersion(&ver);
    ::RxStart();
    if(BuildVersion == ver) {
        addStatusText(tr("Pulling RST low (taking control over bus)."));
        ::SetDirections(SB_TRI);
        ::SetConfig(0,0,0,0,0);
        RxStart();
        QMessageBox::critical(this, tr("%1 information").arg(TARGETNAME),
                              tr("Turn on the device in use and press OK when ready."));
    }
    else
        addStatusText(tr("ProgSkeet is not up to date. Version expected: %1.%2; Version used: %3.%4.")
                        .arg(BuildVersion&0xFF, 8, 10, QChar('0')).toUpper()
                        .arg((BuildVersion>>8)&0xFF, 8, 10, QChar('0')).toUpper()
                        .arg(ver&0xFF, 8, 10, QChar('0')).toUpper()
                        .arg((ver>>8)&0xFF, 8, 10, QChar('0')).toUpper()
                        );

    ::RemoveDevice();
}

void MainWindow::on_btnNandDump_clicked()
{
    if (!dualNandArgs.nand[0].enabled[0] && !dualNandArgs.nand[0].enabled[1] &&
            !dualNandArgs.nand[1].enabled[0] && !dualNandArgs.nand[1].enabled[1]) {
        QMessageBox::critical(this, tr("%1 error").arg(TARGETNAME),
                              tr("No NANDs have been enabled, nothing to do!"));
        return;
    }

    QString filename0, filename1, filename2, filename3;

    DualNandArgs nandArgs = getNandArgs();
    //important

    if(nandArgs.nand[0].enabled[0])
    {
        filename0 = QFileDialog::getSaveFileName(this, tr("Dump destination (NAND A, Primary)..."), 0, FLASH_FILE_FILTER);
        if(filename0 != "")
        {
            QFile fp(filename0);
            if (!fp.open(QIODevice::WriteOnly)) {
                addStatusText(tr("Failed to open file for writing"));
                return;// false;
            }
            else
                fp.close();
        }
        else
            return;
    }
    if(nandArgs.nand[0].enabled[1])
    {
         filename1 = QFileDialog::getSaveFileName(this, tr("Dump destination (NAND A, Secondary)..."), 0, FLASH_FILE_FILTER);

         if(filename1 != "")
         {
             QFile fp(filename1);
             if (!fp.open(QIODevice::WriteOnly)) {
                 addStatusText(tr("Failed to open file for writing"));
                 return;// false;
             }
             else
                 fp.close();
         }
         else
             return;
    }
    if(nandArgs.nand[1].enabled[0])
    {
         filename2 = QFileDialog::getSaveFileName(this, tr("Dump destination (NAND B, Primary)..."), 0, FLASH_FILE_FILTER);
         if(filename2 != "")
         {
             QFile fp(filename2);
             if (!fp.open(QIODevice::WriteOnly)) {
                 addStatusText(tr("Failed to open file for writing"));
                 return;// false;
             }
             else
                 fp.close();
         }
         else
             return;
    }
    if(nandArgs.nand[1].enabled[1])
    {
         filename3 = QFileDialog::getSaveFileName(this, tr("Dump destination (NAND B, Secondary)..."), 0, FLASH_FILE_FILTER);
         if(filename3 != "")
         {
             QFile fp(filename3);
             if (!fp.open(QIODevice::WriteOnly)) {
                 addStatusText(tr("Failed to open file for writing"));
                 return;// false;
             }
             else
                 fp.close();
         }
         else
             return;
    }




    disableButtons(true);
    NandDumpThread* ndt = new NandDumpThread(filename0, filename1, filename2, filename3, commonArgs, dualNandArgs);
    connectFlasherThread(ndt);
    ndt->start();
}

void MainWindow::on_btnNandFlash_clicked()
{
    if (!dualNandArgs.nand[0].enabled[0] && !dualNandArgs.nand[0].enabled[1] &&
            !dualNandArgs.nand[1].enabled[0] && !dualNandArgs.nand[1].enabled[1]) {
        QMessageBox::critical(this, tr("%1 error").arg(TARGETNAME),
                              tr("No NANDs have been enabled, nothing to do!"));
        return;
    }

    QString filename0, filename1, filename2, filename3;

    DualNandArgs nandArgs = getNandArgs();
    //important

    if(nandArgs.nand[0].enabled[0])
    {
        filename0 = QFileDialog::getOpenFileName(this, tr("Flash source (NAND A, Primary)..."), 0, FLASH_FILE_FILTER);
        if(filename0 != "")
        {
            QFile fp(filename0);
            if (!fp.open(QIODevice::ReadOnly)) {
                addStatusText(tr("Failed to open file for reading"));
                return;// false;
            }
            else
                fp.close();
        }
        else
            return;
    }
    if(nandArgs.nand[0].enabled[1])
    {
         filename1 = QFileDialog::getOpenFileName(this, tr("Flash source (NAND A, Secondary)..."), 0, FLASH_FILE_FILTER);

         if(filename1 != "")
         {
             QFile fp(filename1);
             if (!fp.open(QIODevice::ReadOnly)) {
                 addStatusText(tr("Failed to open file for reading"));
                 return;// false;
             }
             else
                 fp.close();
         }
         else
             return;
    }
    if(nandArgs.nand[1].enabled[0])
    {
         filename2 = QFileDialog::getOpenFileName(this, tr("Flash source (NAND B, Primary)..."), 0, FLASH_FILE_FILTER);
         if(filename2 != "")
         {
             QFile fp(filename2);
             if (!fp.open(QIODevice::ReadOnly)) {
                 addStatusText(tr("Failed to open file for reading"));
                 return;// false;
             }
             else
                 fp.close();
         }
         else
             return;
    }
    if(nandArgs.nand[1].enabled[1])
    {
         filename3 = QFileDialog::getOpenFileName(this, tr("Flash source (NAND B, Secondary)..."), 0, FLASH_FILE_FILTER);
         if(filename3 != "")
         {
             QFile fp(filename3);
             if (!fp.open(QIODevice::ReadOnly)) {
                 addStatusText(tr("Failed to open file for reading"));
                 return;// false;
             }
             else
                 fp.close();
         }
         else
             return;
    }

    disableButtons(true);
    NandFlashThread* nft = new NandFlashThread(filename0, filename1, filename2, filename3, commonArgs, nandArgs);
    connectFlasherThread(nft);
    nft->start();
}

void MainWindow::on_btnNandErase_clicked()
{
    disableButtons(true);

    DualNandArgs nandArgs = getNandArgs();
    NandEraseThread* net = new NandEraseThread(commonArgs, nandArgs);
    connectFlasherThread(net);
    net->start();
}
void MainWindow::on_btnAutoNand1_clicked()
{
    if (!::CreateDevice()) {
        QMessageBox::critical(this, tr("%1 read id information").arg(TARGETNAME),
                              tr("Device not found!"));
        return;
    }

    disableButtons(true);
        unsigned char id[4*5];
        QString str;


        QByteArray readBuf0('0', 5);
        QByteArray readBuf1('0', 5);
        QByteArray readBuf2('0', 5);
        QByteArray readBuf3('0', 5);

        ::NAND_Configure(0,0,0,0,0,0);

        char *buffer[4] = {0,0,0,0};

        buffer[0] = dualNandArgs.nand[0].enabled[0]?readBuf0.data():0;
        buffer[1] = dualNandArgs.nand[0].enabled[1]?readBuf1.data():0;
        buffer[2] = dualNandArgs.nand[1].enabled[0]?readBuf2.data():0;
        buffer[3] = dualNandArgs.nand[1].enabled[1]?readBuf3.data():0;
        ::NAND_ReadID(buffer, 5);
        ::RxStart();


            if(dualNandArgs.nand[0].enabled[0])
            {

                str = QString("NAND A [CE_A] ID: %1 %2 %3 %4 %5")
                        .arg(QString::number((unsigned char)readBuf0[0], 16))
                        .arg(QString::number((unsigned char)readBuf0[1], 16))
                        .arg(QString::number((unsigned char)readBuf0[2], 16))
                        .arg(QString::number((unsigned char)readBuf0[3], 16))
                        .arg(QString::number((unsigned char)readBuf0[4], 16));
                addStatusText(str);
            }

            if(dualNandArgs.nand[0].enabled[1])
            {
                str = QString("NAND A [CE_B] ID: %1 %2 %3 %4 %5")
                        .arg(QString::number((unsigned char)readBuf1[0], 16))
                        .arg(QString::number((unsigned char)readBuf1[1], 16))
                        .arg(QString::number((unsigned char)readBuf1[2], 16))
                        .arg(QString::number((unsigned char)readBuf1[3], 16))
                        .arg(QString::number((unsigned char)readBuf1[4], 16));
                addStatusText(str);
            }


            if(dualNandArgs.nand[1].enabled[0])
            {

                str = QString("NAND B [CE_A] ID: %1 %2 %3 %4 %5")
                        .arg(QString::number((unsigned char)readBuf2[0], 16))
                        .arg(QString::number((unsigned char)readBuf2[1], 16))
                        .arg(QString::number((unsigned char)readBuf2[2], 16))
                        .arg(QString::number((unsigned char)readBuf2[3], 16))
                        .arg(QString::number((unsigned char)readBuf2[4], 16));
                addStatusText(str);
            }

            if(dualNandArgs.nand[1].enabled[1])
            {
                str = QString("NAND B [CE_B] ID: %1 %2 %3 %4 %5")
                        .arg(QString::number((unsigned char)readBuf3[0], 16))
                        .arg(QString::number((unsigned char)readBuf3[1], 16))
                        .arg(QString::number((unsigned char)readBuf3[2], 16))
                        .arg(QString::number((unsigned char)readBuf3[3], 16))
                        .arg(QString::number((unsigned char)readBuf3[4], 16));
                addStatusText(str);
            }




        ::RemoveDevice();
        disableButtons(false);
        return;


////	ui->lblIdCode1->setText(str.toUpper());
//	int m;
//	for (m = 0; nand_manuf_ids[m].id != 0x0; m++) {
//		if (nand_manuf_ids[m].id == id[0])
//			break;
//	}
////	ui->lblNand1Maker->setText(QString(nand_manuf_ids[m].name));


//	struct Nand n;
//	n.maker_code = id[0];
//	n.device_id = id[1];
//	n.internal_chip_number = 1<<(id[2]&0x3);
//	n.cell_type = 2<<((id[2]>>2)&0x3);
//	n.num_of_simult_prg_pages = 1<<((id[2]>>4)&0x3);
//	n.interleave_program = (id[2]>>6)&0x1;
//	n.cache_program = (id[2]>>7)&0x1;
//	n.page_size = 1024 <<(id[3] & 0x3);
//	n.spare_512 = 8<<((id[3]&(1<<2))>>2);
//	n.block_size = 0x10000 << ((id[3]>>4)&0x3);
//	n.plane = 1<<((id[4]>>2)&0x3);
//	n.plane_size = 64<<((id[4]>>4)&0x7);
//	n.block_count = (((n.plane * n.plane_size) >> 3) << 10) / (n.block_size >> 10);

// 	// if no __builtin_ffs:
//	//int i,j;
//	//i=0;if(n.page_size) { j=n.block_size/n.page_size; while (j>>=1) { i++; printf("ppb %d %d\n", i, j); } }
//	//ui->cbNand1PagesPerBlock->setCurrentIndex(i);
//        ui->cbNandPagesPerBlock->setCurrentIndex(__builtin_ffs(n.block_size/n.page_size?:1)-1);
// 	// if no __builtin_ffs:
//	//i=0;j=n.block_count; while (j>>=1) { i++; printf("blockcount %d %d\n", i, j); }
//	//ui->cbNand1BlockCount->setCurrentIndex(i);
//        ui->cbNandBlockCount->setCurrentIndex(__builtin_ffs(n.block_count)-1);
//        ui->cboxNandBigBlock->setChecked(n.page_size>1024?true:false);

}
void MainWindow::on_btnAutoNand2_clicked()
{
//      if (!::CreateDevice()) {
//        QMessageBox::critical(this, tr("%1 read id information").arg(TARGETNAME),
//                              tr("Device not found!."));
//        return;
//    }
//	unsigned char id[5];
//	QString str;
//        ::NAND_ReadID((char *)id, (char)5, 1, 1);
//	str = QString("%1%2%3%4%5")
//		.arg(id[0], 2, 16, QChar('0'))
//		.arg(id[1], 2, 16, QChar('0'))
//		.arg(id[2], 2, 16, QChar('0'))
//		.arg(id[3], 2, 16, QChar('0'))
//		.arg(id[4], 2, 16, QChar('0'));
////	ui->lblIdCode2->setText(str.toUpper());
//	int m;
//	for (m = 0; nand_manuf_ids[m].id != 0x0; m++) {
//		if (nand_manuf_ids[m].id == id[0])
//			break;
//	}
//	ui->lblNand2Maker->setText(QString(nand_manuf_ids[m].name));


//	struct Nand n;
//	n.maker_code = id[0];
//	n.device_id = id[1];
//	n.internal_chip_number = 1<<(id[2]&0x3);
//	n.cell_type = 2<<((id[2]>>2)&0x3);
//	n.num_of_simult_prg_pages = 1<<((id[2]>>4)&0x3);
//	n.interleave_program = (id[2]>>6)&0x1;
//	n.cache_program = (id[2]>>7)&0x1;
//	n.page_size = 1024 <<(id[3] & 0x3);
//	n.spare_512 = 8<<((id[3]&(1<<2))>>2);
//	n.block_size = 0x10000 << ((id[3]>>4)&0x3);
//	n.plane = 1<<((id[4]>>2)&0x3);
//	n.plane_size = 64<<((id[4]>>4)&0x7);
//	n.block_count = (((n.plane * n.plane_size) >> 3) << 10) / (n.block_size >> 10);

// 	// if no __builtin_ffs:
//	//int i,j;
//	//i=0;if(n.page_size) { j=n.block_size/n.page_size; while (j>>=1) { i++; printf("ppb %d %d\n", i, j); } }
//	//ui->cbNand2PagesPerBlock->setCurrentIndex(i);
//	ui->cbNand2PagesPerBlock->setCurrentIndex(__builtin_ffs(n.block_size/n.page_size?:1)-1);
// 	// if no __builtin_ffs:
//	//i=0;j=n.block_count; while (j>>=1) { i++; printf("blockcount %d %d\n", i, j); }
//	//ui->cbNand2BlockCount->setCurrentIndex(i);
//	ui->cbNand2BlockCount->setCurrentIndex(__builtin_ffs(n.block_count)-1);
//        ui->cboxNand2BigBlock->setChecked(n.page_size>1024?true:false);

//	::RemoveDevice();

}

void MainWindow::on_cbNand1BlockCount_currentIndexChanged(int index)
{
//    dualNandArgs.nand[0].blockCount = 1 << index;

//    ui->cboxNandCustomRange->setChecked(false);

//    dualNandArgs.nand[0].range.start = 0;
//    dualNandArgs.nand[0].range.end = (1 << index) - 1;
}

void MainWindow::on_cboxNandCustomRange_toggled(bool checked)
{
    ui->frmNandCustomRange->setEnabled(checked);

    if (checked) {
        ui->cbNandBlockStart->clear();
        ui->cbNandBlockEnd->clear();

        QString text;
        int curIdx = ui->cbNandBlockCount->currentIndex();
        for (int i = 0; i < (1 << curIdx); i++) {
            text = QString::number(i);
            ui->cbNandBlockStart->addItem(text);
            ui->cbNandBlockEnd->addItem(text);
        }

        ui->cbNandBlockStart->setCurrentIndex(0);
        ui->cbNandBlockEnd->setCurrentIndex(ui->cbNandBlockEnd->count() - 1);
    }
}

void MainWindow::on_cbNand2BlockCount_currentIndexChanged(int index)
{
//    dualNandArgs.nand[1].blockCount = 1 << index;

//    ui->cboxNand2CustomRange->setChecked(false);

//    dualNandArgs.nand[1].range.start = 0;
//    dualNandArgs.nand[1].range.end = (1 << index) - 1;
}

void MainWindow::on_cboxNand2CustomRange_toggled(bool checked)
{
//    ui->frmNand2CustomRange->setEnabled(checked);

//    if (checked) {
//        ui->cbNand2BlockStart->clear();
//        ui->cbNand2BlockEnd->clear();

//        QString text;
//        int curIdx = ui->cbNand2BlockCount->currentIndex();
//        for (int i = 0; i < (1 << curIdx); i++) {
//            text = QString::number(i);
//            ui->cbNand2BlockStart->addItem(text);
//            ui->cbNand2BlockEnd->addItem(text);
//        }

//        ui->cbNand2BlockStart->setCurrentIndex(0);
//        ui->cbNand2BlockEnd->setCurrentIndex(ui->cbNand2BlockEnd->count() - 1);
//    }
}

void MainWindow::setProgressText(const QString& text)
{
    //void progressSetText(const QString& text);
    ui->progressBar->setFormat(text);
}

void MainWindow::setProgress(const int cur)
{
    ui->progressBar->setValue(cur);
}

void MainWindow::UpdateRxSpeed(const int value)
{
    ui->lcdRxSpeed->display(value);
}

void MainWindow::UpdateTxSpeed(const int value)
{
    ui->lcdTxSpeed->display(value);
}


void MainWindow::addStatusText(const QString& text)
{
#ifdef DEBUG
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss.zzz");
#else
    QString time = QDateTime::currentDateTime().toString(Qt::ISODate);
#endif

    ui->logBox->appendPlainText(QString("[%1] %2").arg(time).arg(text));
}

void MainWindow::onFinished()
{
    disableButtons(false);
}

void MainWindow::connectFlasherThread(FlasherThread* flasherThread)
{
    QObject::connect(flasherThread, SIGNAL(progressChanged(const int)),
        this, SLOT(setProgress(const int)), Qt::QueuedConnection);

    QObject::connect(flasherThread, SIGNAL(progressRangeChanged(const int, const int)),
        this, SLOT(setProgressRange(const int, const int)), Qt::QueuedConnection);

    QObject::connect(flasherThread, SIGNAL(statusTextAdded(const QString&)),
        this, SLOT(addStatusText(const QString&)), Qt::QueuedConnection);

    QObject::connect(flasherThread, SIGNAL(finished()),
        this, SLOT(onFinished()), Qt::QueuedConnection);

    QObject::connect(ui->btnCancel, SIGNAL(clicked()),
        flasherThread, SLOT(onCanceled()));

    QObject::connect(flasherThread, SIGNAL(TxSpeedUpdate(const int)),
        this, SLOT(UpdateTxSpeed(const int)), Qt::QueuedConnection);

    QObject::connect(flasherThread, SIGNAL(RxSpeedUpdate(const int)),
        this, SLOT(UpdateRxSpeed(const int)), Qt::QueuedConnection);

    QObject::connect(flasherThread, SIGNAL(progressSetText(const QString&)),
        this, SLOT(setProgressText(const QString&)), Qt::QueuedConnection);
}

void MainWindow::disableButtons(bool disabled)
{
    ui->btnNorDump->setDisabled(disabled);
    ui->btnNorFlash->setDisabled(disabled);
    ui->btnNorErase->setDisabled(disabled);
    ui->btnNorDumpCFI->setDisabled(disabled);
    ui->btnNorDetect->setDisabled(disabled);

    ui->btnNandDump->setDisabled(disabled);
    ui->btnNandFlash->setDisabled(disabled);
    ui->btnNandErase->setDisabled(disabled);

    //ui->btnTestShorts->setDisabled(disabled);
    ui->btnAutoNand1->setDisabled(disabled);

    /* Cancel is enabled when an operation is running */
    ui->btnCancel->setDisabled(!disabled);
}

void MainWindow::setProgressRange(const int min, const int max)
{
    ui->progressBar->setMinimum(min);
    ui->progressBar->setMaximum(max);
}

void MainWindow::on_btnClear_clicked()
{
    ui->logBox->clear();
}

void MainWindow::on_cbNand1BlockStart_currentIndexChanged(int index)
{
//    if (index > ui->cbNandBlockEnd->currentIndex())
//        ui->cbNandBlockEnd->setCurrentIndex(index);

//    dualNandArgs.nand[0].range.start = index;
}

void MainWindow::on_cbNand1BlockEnd_currentIndexChanged(int index)
{
//    if (index < ui->cbNandBlockStart->currentIndex())
//        ui->cbNandBlockStart->setCurrentIndex(index);

//    dualNandArgs.nand[0].range.end = index;
}

void MainWindow::on_cbNand2BlockStart_currentIndexChanged(int index)
{
//    if (index > ui->cbNand2BlockEnd->currentIndex())
//        ui->cbNand2BlockEnd->setCurrentIndex(index);

//    dualNandArgs.nand[1].range.start = index;
}

void MainWindow::on_cbNand2BlockEnd_currentIndexChanged(int index)
{
//    if (index < ui->cbNand2BlockStart->currentIndex())
//        ui->cbNand2BlockStart->setCurrentIndex(index);

//    dualNandArgs.nand[1].range.end = index;
}

void MainWindow::on_cboxNand1BigBlock_toggled(bool checked)
{
//    dualNandArgs.nand[0].bigBlock = checked;
}

void MainWindow::on_cboxNand1Raw_toggled(bool checked)
{
//    dualNandArgs.nand[0].raw = checked;
}

void MainWindow::on_cboxNand2BigBlock_toggled(bool checked)
{
//    dualNandArgs.nand[1].bigBlock = checked;
}

void MainWindow::on_cboxNand2Raw_toggled(bool checked)
{
//    dualNandArgs.nand[0].raw = checked;
//    dualNandArgs.nand[1].raw = checked;
}

void MainWindow::on_cbNand1PagesPerBlock_currentIndexChanged(int index)
{
//    dualNandArgs.Settings.pageCount = index;
}

//void MainWindow::on_cbNand2PagesPerBlock_currentIndexChanged(int index)
//{
//    dualNandArgs.nand[1].pageCount = index;
//}

void MainWindow::on_cbPeriod_currentIndexChanged(int index)
{
    commonArgs.period = index;
}

//void MainWindow::on_btnTestShorts_clicked()
//{
//    if (!::CreateDevice()) {
//        QMessageBox::critical(this, tr("%1 shorts information").arg(TARGETNAME),
//                              tr("Device not found! Please check all leads on U2 and leads 50 through 75 on U1."));
//        return;
//    }

//    quint32 shorts = ::Debug_TestShorts();

//    ::RemoveDevice();

//    if (shorts == 0) {
//        QMessageBox::information(this, tr("%1 shorts information").arg(TARGETNAME),
//                                 tr("No shorts found! Device is properly working."));
//        return;
//    }

//    QStringList shortStrings;

//    for (quint8 i = 0; i < 16; i++) {
//        if (shorts & (1 << i))
//            shortStrings.append(QString("DQ%1").arg(i));
//    }

//    for (quint8 i = 16; i < 32; i++) {
//        if (shorts & (1 << i))
//            shortStrings.append(QString("GP%1").arg(i - 16));
//    }

//    QMessageBox::warning(this, tr("%1 shorts information").arg(TARGETNAME),
//                         tr("Shorts found! Please check %1!").arg(shortStrings.join(", ")));

//}

void MainWindow::on_cboxDouble_toggled(bool checked)
{
    commonArgs.word = checked;
    ui->cboxByteSwap->setChecked(false);
    ui->cboxByteSwap->setEnabled(checked);
}

DualNandArgs MainWindow::getNandArgs()
{
    return dualNandArgs;
}

void MainWindow::presetsInit()
{
    ui->cbPresets->addItem(tr("--- Predefined ---"), -1);

    ui->cbPresets->addItem(tr("Default settings"), -1);
    ui->cbPresets->addItem(tr("NOR Spansion S29GL128N90TFIR2"), -1);
    ui->cbPresets->addItem(tr("NOR Spansion S29GL128P90TFIR2"), -1);
    ui->cbPresets->addItem(tr("NOR Macronix MX29GL128ELT2I-90G"), -1);
    ui->cbPresets->addItem(tr("NOR Samsung K8P2716UZC-QI4D"), -1);
    ui->cbPresets->addItem(tr("NOR Samsung K8Q2815UQB-PI4B"), -1);
    ui->cbPresets->addItem(tr("Dual NAND Samsung K9F1G08U0A/B-PIB0"), -1);
    ui->cbPresets->addItem(tr("NAND HYNIX HY27US08281A"), -1);

    ui->cbPresets->addItem(tr("--- User defined ---"), -1);

    ui->cbPresets->insertSeparator(ui->cbPresets->count());

    settings.beginGroup("presets");

    QStringList presetKeys = settings.childGroups();
    for (int i = 0; i < presetKeys.size(); ++i) {
        QString presetKey = presetKeys.at(i);
        int presetIndex = presetKey.toInt();
        QString presetName = settings.value(presetKey + "/Name").toString();
        ui->cbPresets->addItem(presetName, presetIndex);
    }

    settings.endGroup();

    /* Load the default settings */
    ui->cbPresets->setCurrentIndex(1);
}

int MainWindow::presetIndexNext()
{
    settings.beginGroup("presets");

    QStringList presetKeys = settings.childGroups();
    int i;
    for (i = 0; i < presetKeys.size(); ++i) {
        QString presetKey = presetKeys.at(i);
        int presetIndex = presetKey.toInt();
        if (presetIndex != i)
            break;
    }

    settings.endGroup();

    return i;
}

int MainWindow::presetNew(const QString& name)
{
    int index = presetIndexNext();

    settings.beginGroup("presets");
    settings.setValue(QString::number(index) + "/Name", name);
    settings.endGroup();

    presetSave(index);

    return index;
}

void MainWindow::presetSave(int index)
{
    settings.beginGroup("presets/" + QString::number(index));

    /* Common */
    settings.setValue("Differential", ui->cboxDifferential->isChecked());
    settings.setValue("Verify", ui->cboxVerify->isChecked());
    settings.setValue("AbortOnError", ui->cboxAbortOnError->isChecked());
    settings.setValue("Double", ui->cboxDouble->isChecked());
    settings.setValue("ByteSwap", ui->cboxByteSwap->isChecked());
    //settings.setValue("Period", ui->cbPeriod->currentIndex());
    //settings.setValue("ReadPageDelay", ui->inputReadDelay->text());

    /* NOR */
    settings.setValue("NorDeviceSize", ui->spNorDeviceSize->value());
    settings.setValue("NorChipCount", ui->spNorChipCount->value());

    settings.setValue("NorRegion1BlockSize", ui->spNorRegion1BlockSize->value());
    settings.setValue("NorRegion1BlockCount", ui->spNorRegion1BlockCount->value());
    settings.setValue("NorRegion2Enabled", ui->cboxNorRegion2->isChecked());
    settings.setValue("NorRegion2BlockSize", ui->spNorRegion2BlockSize->value());
    settings.setValue("NorRegion2BlockCount", ui->spNorRegion2BlockCount->value());
    settings.setValue("NorRegion3Enabled", ui->cboxNorRegion3->isChecked());
    settings.setValue("NorRegion3BlockSize", ui->spNorRegion3BlockSize->value());
    settings.setValue("NorRegion3BlockCount", ui->spNorRegion3BlockCount->value());
    settings.setValue("NorRegion4Enabled", ui->cboxNorRegion4->isChecked());
    settings.setValue("NorRegion4BlockSize", ui->spNorRegion4BlockSize->value());
    settings.setValue("NorRegion4BlockCount", ui->spNorRegion4BlockCount->value());

    settings.setValue("NorRangeStartEnabled", ui->cboxNorRangeStart->isChecked());
    settings.setValue("NorRangeStart", ui->spNorRangeStart->value());
    settings.setValue("NorRangeEndEnabled", ui->cboxNorRangeEnd->isChecked());
    settings.setValue("NorRangeEnd", ui->spNorRangeEnd->value());
    settings.setValue("NorRangeSync", ui->cboxNorRangeSync->isChecked());

    settings.setValue("NorWriteMethod", ui->cbNorWriteMethod->currentIndex());
    settings.setValue("NorBufferedMaxBytes", ui->spNorBufferedMaxBytes->value());
    settings.setValue("NorWaitMethod", ui->cbNorWaitMethod->currentIndex());

    /* NAND 1 */
    //settings.setValue("Nand1", ui->cboxNand1->isChecked());
    settings.setValue("NandBlockSize", ui->cbNandPagesPerBlock->currentIndex());
    settings.setValue("NandBlockCount", ui->cbNandBlockCount->currentIndex());
    settings.setValue("NandCustomRange", ui->cboxNandCustomRange->isChecked());
    settings.setValue("NandBlockStart", ui->cbNandBlockStart->currentIndex());
    settings.setValue("NandBlockEnd", ui->cbNandBlockEnd->currentIndex());
    settings.setValue("NandBigBlock", ui->cboxNandBigBlock->isChecked());
    settings.setValue("NandRaw", ui->cboxNandRaw->isChecked());

    /* NAND 2 */
//    settings.setValue("Nand2", ui->cboxNand2->isChecked());
//    settings.setValue("Nand2BlockSize", ui->cbNand2PagesPerBlock->currentIndex());
//    settings.setValue("Nand2BlockCount", ui->cbNand2BlockCount->currentIndex());
//    settings.setValue("Nand2CustomRange", ui->cboxNand2CustomRange->isChecked());
//    settings.setValue("Nand2BlockStart", ui->cbNand2BlockStart->currentIndex());
//    settings.setValue("Nand2BlockEnd", ui->cbNand2BlockEnd->currentIndex());
//    settings.setValue("Nand2BigBlock", ui->cboxNand2BigBlock->isChecked());
//    settings.setValue("Nand2Raw", ui->cboxNand2Raw->isChecked());

    settings.endGroup();
}

void MainWindow::presetLoad(const int index)
{
    settings.beginGroup("presets/" + QString::number(index));

    /* Common */
    ui->cboxDifferential->setChecked(settings.value("Differential", true).toBool());
    ui->cboxVerify->setChecked(settings.value("Verify", true).toBool());
    ui->cboxAbortOnError->setChecked(settings.value("AbortOnError", false).toBool());
    ui->cboxDouble->setChecked(settings.value("Double", true).toBool());
    ui->cboxByteSwap->setChecked(settings.value("ByteSwap", false).toBool());
    //ui->cbPeriod->setCurrentIndex(settings.value("Period", 5).toInt());

    /* NOR */
    ui->spNorDeviceSize->setValue(settings.value("NorDeviceSize", 20).toInt());
    ui->spNorChipCount->setValue(settings.value("NorChipCount", 1).toInt());

    ui->spNorRegion1BlockSize->setValue(settings.value("NorRegion1BlockSize", 131072).toInt());
    ui->spNorRegion1BlockCount->setValue(settings.value("NorRegion1BlockCount", 8).toInt());
    ui->cboxNorRegion2->setChecked(settings.value("NorRegion2Enabled", false).toBool());
    ui->spNorRegion2BlockSize->setValue(settings.value("NorRegion2BlockSize", 256).toInt());
    ui->spNorRegion2BlockCount->setValue(settings.value("NorRegion2BlockCount", 1).toInt());
    ui->cboxNorRegion3->setChecked(settings.value("NorRegion3Enabled", false).toBool());
    ui->spNorRegion3BlockSize->setValue(settings.value("NorRegion3BlockSize", 256).toInt());
    ui->spNorRegion3BlockCount->setValue(settings.value("NorRegion3BlockCount", 1).toInt());
    ui->cboxNorRegion4->setChecked(settings.value("NorRegion4Enabled", false).toBool());
    ui->spNorRegion4BlockSize->setValue(settings.value("NorRegion4BlockSize", 256).toInt());
    ui->spNorRegion4BlockCount->setValue(settings.value("NorRegion4BlockCount", 1).toInt());

    ui->cboxNorRangeStart->setChecked(settings.value("NorRangeStartEnabled", false).toBool());
    ui->spNorRangeStart->setValue(settings.value("NorRangeStart", 0).toInt());
    ui->cboxNorRangeEnd->setChecked(settings.value("NorRangeEndEnabled", false).toBool());
    ui->spNorRangeEnd->setValue(settings.value("NorRangeEnd", 0).toInt());
    ui->cboxNorRangeSync->setChecked(settings.value("NorRangeSync", false).toBool());

    ui->cbNorWriteMethod->setCurrentIndex(settings.value("NorWriteMethod", 0).toInt());
    ui->spNorBufferedMaxBytes->setValue(settings.value("NorBufferedMaxBytes", 6).toInt());
    ui->cbNorWaitMethod->setCurrentIndex(settings.value("NorWaitMethod", 0).toInt());

    /* NAND 1 */
    //ui->cboxNand1->setChecked(settings.value("Nand1", false).toBool());
    ui->cbNandPagesPerBlock->setCurrentIndex(settings.value("NandBlockSize", 0).toInt());
    ui->cbNandBlockCount->setCurrentIndex(settings.value("NandBlockCount", 0).toInt());
    ui->cboxNandCustomRange->setChecked(settings.value("NandCustomRange", false).toBool());
    ui->cbNandBlockStart->setCurrentIndex(settings.value("NandBlockStart", 0).toInt());
    ui->cbNandBlockEnd->setCurrentIndex(settings.value("NandBlockEnd", 0).toInt());
    ui->cboxNandBigBlock->setChecked(settings.value("NandBigBlock", false).toBool());
    ui->cboxNandRaw->setChecked(settings.value("NandRaw", false).toBool());

    /* NAND 2 */
//    ui->cboxNand2->setChecked(settings.value("Nand2", false).toBool());
//    ui->cbNand2PagesPerBlock->setCurrentIndex(settings.value("Nand2BlockSize", 0).toInt());
//    ui->cbNand2BlockCount->setCurrentIndex(settings.value("Nand2BlockCount", 0).toInt());
//    ui->cboxNand2CustomRange->setChecked(settings.value("Nand2CustomRange", false).toBool());
//    ui->cbNand2BlockStart->setCurrentIndex(settings.value("Nand2BlockStart", 0).toInt());
//    ui->cbNand2BlockEnd->setCurrentIndex(settings.value("Nand2BlockEnd", 0).toInt());
//    ui->cboxNand2BigBlock->setChecked(settings.value("Nand2BigBlock", false).toBool());
//    ui->cboxNand2Raw->setChecked(settings.value("Nand2Raw", false).toBool());

    settings.endGroup();

    updateCustomRange();
}

void MainWindow::presetDelete(const int index)
{
    settings.beginGroup("presets");
    settings.remove(QString::number(index));
    settings.endGroup();
}

void MainWindow::on_cbPresets_currentIndexChanged(int index)
{
    if (index < 1)
        return;

    int setIdx = ui->cbPresets->itemData(index).toInt();
    if (setIdx != -1) {
        presetLoad(setIdx);
        ui->btnPresetSave->setEnabled(true);
        ui->btnPresetDelete->setEnabled(true);
        return;
    }

    ui->btnPresetSave->setEnabled(false);
    ui->btnPresetDelete->setEnabled(false);

    switch (index) {
    case 1: /* Default settings */
        presetLoad(-1);
        break;
    case 2: /* NOR Spansion S29GL128N90TFIR2 */
        ui->cboxDouble->setChecked(true);
        ui->cboxByteSwap->setChecked(true);

        ui->spNorDeviceSize->setValue(24);
        ui->spNorChipCount->setValue(1);

        ui->spNorRegion1BlockSize->setValue(131072); /* 128 KiB */
        ui->spNorRegion1BlockCount->setValue(128); /* 128 x 128 KiB = 16 MiB */

        ui->cboxNorRegion2->setChecked(false);

        ui->cboxNorRegion3->setChecked(false);

        ui->cboxNorRegion4->setChecked(false);

        ui->cbNorWriteMethod->setCurrentIndex((int)NorWriteMethod_BufferedWrite);
        ui->spNorBufferedMaxBytes->setValue(5); /* 32 bytes per buffered write */
        break;
    case 3: /* NOR Spansion S29GL128P90TFIR2 */
    case 4: /* NOR Macronix MX29GL128ELT2I-90G */
    case 5: /* NOR Samsung K8P2716UZC-QI4D */
        ui->cboxDouble->setChecked(true);
        ui->cboxByteSwap->setChecked(true);

        ui->spNorDeviceSize->setValue(24);
        ui->spNorChipCount->setValue(1);

        ui->spNorRegion1BlockSize->setValue(131072); /* 128 KiB */
        ui->spNorRegion1BlockCount->setValue(128); /* 128 x 128 KiB = 16 MiB */

        ui->cboxNorRegion2->setChecked(false);

        ui->cboxNorRegion3->setChecked(false);

        ui->cboxNorRegion4->setChecked(false);

        ui->cbNorWriteMethod->setCurrentIndex((int)NorWriteMethod_BufferedWrite);
        ui->spNorBufferedMaxBytes->setValue(6); /* 64 bytes per buffered write */
        break;
    case 6: /* NOR Samsung K8Q2815UQB-PI4B */
        ui->cboxDouble->setChecked(true);
        ui->cboxByteSwap->setChecked(true);

        ui->spNorDeviceSize->setValue(23);
        ui->spNorChipCount->setValue(2);

        ui->spNorRegion1BlockSize->setValue(8192); /* 8 KiB */
        ui->spNorRegion1BlockCount->setValue(8); /* 8 x 8 KiB = 64 KiB */

        ui->cboxNorRegion2->setChecked(true);
        ui->spNorRegion2BlockSize->setValue(65536); /* 64 KiB */
        ui->spNorRegion2BlockCount->setValue(126); /* 126 x 64 KiB = 8064 KiB */

        ui->cboxNorRegion3->setChecked(true);
        ui->spNorRegion3BlockSize->setValue(8192); /* 8 KiB */
        ui->spNorRegion3BlockCount->setValue(8); /* 8 x 8 KiB = 64 KiB */

        ui->cboxNorRegion4->setChecked(false);

        ui->cbNorWriteMethod->setCurrentIndex((int)NorWriteMethod_SingleWordProgram);
        break;
    case 7: /* Dual NAND Samsung K9F1G08U0A-PIB0 / K9F1G08UOB-PIB0 */
        ui->cboxDouble->setChecked(false);
        ui->cboxByteSwap->setChecked(false);

        //ui->cboxNand1->setChecked(true);
        ui->cboxNandBigBlock->setChecked(true);
        ui->cboxNandRaw->setChecked(true);
        ui->cbNandPagesPerBlock->setCurrentIndex(6);
        ui->cbNandBlockCount->setCurrentIndex(10);

        //ui->cboxNand2->setChecked(true);
//        ui->cboxNand2BigBlock->setChecked(true);
//        ui->cboxNand2Raw->setChecked(true);
//        ui->cbNand2PagesPerBlock->setCurrentIndex(6);
//        ui->cbNand2BlockCount->setCurrentIndex(10);
        break;
    case 8: /* NAND HYNIX HY27US08281A */
        ui->cboxDouble->setChecked(false);
        ui->cboxByteSwap->setChecked(false);
        //ui->cboxNand1->setChecked(true);
        ui->cboxNandBigBlock->setChecked(false);
        //ui->cboxNand1Raw->setChecked(true);
        ui->cbNandPagesPerBlock->setCurrentIndex(5);
        ui->cbNandBlockCount->setCurrentIndex(10);
        //ui->cboxNand2->setChecked(false);
        break;
    default:
        break;
    }
}

void MainWindow::on_btnPresetNew_clicked()
{
    QString presetName = QInputDialog::getText(this, TARGETNAME, tr("Name for preset:"));
    if (presetName.isEmpty())
        return;

    int presetIndex = presetNew(presetName);

    ui->cbPresets->addItem(presetName, presetIndex);
    ui->cbPresets->setCurrentIndex(ui->cbPresets->count() - 1);
}

void MainWindow::on_btnPresetSave_clicked()
{
    int index = ui->cbPresets->currentIndex();
    if (index < 1)
        return;

    int presetIndex = ui->cbPresets->itemData(index).toInt();
    if (presetIndex == -1)
        return;

    presetSave(presetIndex);
}

void MainWindow::on_btnPresetDelete_clicked()
{
    int index = ui->cbPresets->currentIndex();
    if (index < 1)
        return;

    int presetIndex = ui->cbPresets->itemData(index).toInt();
    if (presetIndex == -1)
        return;

    presetDelete(presetIndex);

    ui->cbPresets->removeItem(index);
}

void MainWindow::on_btnInputFileBrowse_clicked()
{
    ui->lineInputFile->setText(QFileDialog::getOpenFileName(this, tr("Patch input file..."), 0, FLASH_FILE_FILTER));
}

void MainWindow::on_btnPatchFileBrowse_clicked()
{
    ui->linePatchFile->setText(QFileDialog::getOpenFileName(this, tr("Patch file..."), 0, PATCH_FILE_FILTER));
}

void MainWindow::on_btnApply_clicked()
{
    /*
     * Get all the needed filenames
     */
    QString inputFilePath = ui->lineInputFile->text();
    if (inputFilePath.isEmpty()) {
        QMessageBox::critical(this, tr("%1 error").arg(TARGETNAME), tr("No input file selected!"));
        return;
    }

    QString patchFilePath = ui->linePatchFile->text();
    if (patchFilePath.isEmpty()) {
        QMessageBox::critical(this, tr("%1 error").arg(TARGETNAME), tr("No patch file selected!"));
        return;
    }

    QString outFilePath = QFileDialog::getSaveFileName(this, tr("Patch output file..."), 0, FLASH_FILE_FILTER);
    if(outFilePath.isEmpty()) {
        QMessageBox::critical(this, tr("%1 error").arg(TARGETNAME), tr("No patch output file selected!"));
        return;
    }

    /* Remove output file if it exists, the user has already chosen to overwrite in the save dialog */
    if (QFile::exists(outFilePath))
        QFile::remove(outFilePath);

    /* Copy the input file to the output file */
    if (!QFile::copy(inputFilePath, outFilePath)) {
        QMessageBox::critical(this, tr("%1 error").arg(TARGETNAME), tr("Failed to copy input file to output file!"));
        return;
    }

    QFile patchFile(patchFilePath);
    if (!patchFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("%1 error").arg(TARGETNAME), tr("Failed to open patch file for reading!"));
        return;
    }

    QFileInfo patchFileInfo(patchFilePath);

    QFile outFile(outFilePath);
    if (!outFile.open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, tr("%1 error").arg(TARGETNAME), tr("Failed to open output file for writing!"));
        patchFile.close();
        return;
    }

    QTextStream patchFileStream(&patchFile);
    while (!patchFileStream.atEnd()) {
        /*
         * Split and sanity check the input line at the semicolons
         * The format is [filename];[offset];[length]
         */
        QString patchLine = patchFileStream.readLine();
        QStringList patchLineSplit = patchLine.split(';');

        if (patchLineSplit.count() != 3) {
            QMessageBox::warning(this, tr("%1 warning").arg(TARGETNAME), tr("Encountered malformed line, skipping!"));
            continue;
        }

        quint64 offset, length;
        bool ok;

        offset = patchLineSplit.at(1).toULongLong(&ok, 16);
        if (!ok) {
            QMessageBox::warning(this, tr("%1 warning").arg(TARGETNAME), tr("Encountered malformed offset, skipping!"));
            continue;
        }

        length = patchLineSplit.at(2).toULongLong(&ok, 16);
        if (!ok || length == 0) {
            QMessageBox::warning(this, tr("%1 warning").arg(TARGETNAME), tr("Encountered malformed length, skipping!"));
            continue;
        }

        /* Open the filename relative to the patch file directory */
        QFileInfo patchLineFileInfo(patchFileInfo.dir(), patchLineSplit.at(0));
        QFile patchLineFile(patchLineFileInfo.filePath());
        if (!patchLineFile.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("%1 warning").arg(TARGETNAME),
                                 tr("Failed to find patch file \"%1\", skipping!").arg(patchLineFileInfo.path()));
            continue;
        }

        QByteArray patchData = patchLineFile.read(length);
        if ((quint64)patchData.length() != length) {
            QMessageBox::warning(this, tr("%1 warning").arg(TARGETNAME), tr("Patch file is too short, skipping!"));
            continue;
        }

        outFile.seek(offset);
        outFile.write(patchData);

        patchLineFile.close();
    }

    outFile.close();
    patchFile.close();

    QMessageBox::information(this, TARGETNAME, tr("Patching was successful!"));
}

void MainWindow::on_cboxNand1_toggled(bool checked)
{
//    dualNandArgs.nand[0].enabled = checked;
//    ui->frmNand1->setEnabled(checked);
//    ui->cboxNand1CE_A->setChecked(checked);
}

void MainWindow::on_cboxNand2_toggled(bool checked)
{
//    dualNandArgs.nand[1].enabled = checked;
//    ui->frmNand2->setEnabled(checked);
//    ui->cboxNand2CE_A->setChecked(checked);
}

uint32_t intFromString(const QString& input)
{
    uint32_t ret;

    if (input[0] == '0' && (input[1] == 'x' || input[1] == 'X')) {
        /* Base 16 */
        QString hex = input.mid(2);
        ret = hex.toUInt(NULL, 16);
    } else if (input[0] == 'b') {
        /* Base 2 */
        QString bin = input.mid(1);
        ret = bin.toUInt(NULL, 2);
    } else {
        /* Default to base 10 */
        ret = input.toUInt();
    }

    return ret;
}

//void MainWindow::on_btnSetAddr_clicked()
//{
//    /* TODO: error handling */

//    uint32_t addr = intFromString(ui->addrEdit->text());

//    if (!::CreateDevice())
//        return;

//    ::SetConfig(10, 1, 0, 0, 0);
//    ::NOR_Reset();
//    ::SetAddress(addr, 0);
//    ::TxStart();

//    ::RemoveDevice();
//}

//void MainWindow::on_btnSetData_clicked()
//{
//    /* TODO: error handling */

//    uint16_t data = (intFromString(ui->dataEdit->text()) & 0xFFFF);

//    if (!::CreateDevice())
//        return;

//    ::SetConfig(10, 1, 0, 0, 0);
//    ::NOR_Reset();
//    ::SetData(data);
//    ::TxStart();

//    ::RemoveDevice();
//}

void MainWindow::updateTotals(const BinSizeSpinBox* size, const QSpinBox* count, QLabel* label)
{
    if (!size || !count || !label)
        return;

    uint64_t total = ((1ull << (uint64_t)size->value()) * (uint64_t)count->value());
    label->setText(tr("%1 total").arg(SizeSpinBox::formatSize(total)));
}

void MainWindow::updateTotals(const QSpinBox* size, const QSpinBox* count, QLabel* label)
{
    if (!size || !count || !label)
        return;

    uint64_t total = ((uint64_t)size->value() * (uint64_t)count->value());
    label->setText(tr("%1 total").arg(SizeSpinBox::formatSize(total)));
}

void MainWindow::on_spNorDeviceSize_valueChanged(int arg1)
{
    updateTotals(ui->spNorDeviceSize, ui->spNorChipCount, ui->lblNorDeviceSizeTotal);

    norArgs.deviceSize = arg1;

    ui->spNorRangeStart->setDeviceSize(arg1);
    ui->spNorRangeEnd->setDeviceSize(arg1);

    updateCustomRange();
}

void MainWindow::on_inputReadDelay_textChanged(QString arg1)
{
//    bool ok;
//    int val;
//    val = arg1.toUInt(&ok, 10);
//    if (ok) {
//        dualNandArgs.nand[0].read_page_delay_us = val;
//	dualNandArgs.nand[1].read_page_delay_us = val;
//    }
}

void MainWindow::on_spNorChipCount_valueChanged(int arg1)
{
    updateTotals(ui->spNorDeviceSize, ui->spNorChipCount, ui->lblNorDeviceSizeTotal);

    norArgs.chipCount = arg1;

    ui->spNorRangeStart->setChipCount(arg1);
    ui->spNorRangeEnd->setChipCount(arg1);

    updateCustomRange();
}

void MainWindow::updateEraseBlockRegion(EraseBlockRegion& ebr, const QSpinBox* size, const QSpinBox* count, QLabel* label)
{
    ebr.blockSize = size->value();
    ebr.blockCount = count->value();
    updateTotals(size, count, label);
}

void MainWindow::updateEraseBlockRegions()
{
    norArgs.regions.count = 1;

    if (ui->cboxNorRegion2->isChecked())
        norArgs.regions.count++;

    if (ui->cboxNorRegion3->isChecked())
        norArgs.regions.count++;

    if (ui->cboxNorRegion4->isChecked())
        norArgs.regions.count++;
}

void MainWindow::updateCustomRange()
{
    ui->spNorRangeEnd->setMinimum(
                FlashUtilities::NextEBStart(
                    &norArgs.regions, norArgs.range.start
                    ));

    if (!norArgs.range.startEnabled)
        ui->spNorRangeStart->setValue(0);

    if (!norArgs.range.endEnabled)
        ui->spNorRangeEnd->setValue((1 << norArgs.deviceSize) * norArgs.chipCount);

    uint32_t total = (norArgs.range.end - norArgs.range.start);
    ui->lblNorRangeSize->setText(tr("%1 total").arg(SizeSpinBox::formatSize(total)));
}

void MainWindow::on_cboxNorRegion2_toggled(bool checked)
{
    ui->spNorRegion2BlockSize->setEnabled(checked);
    ui->lblNorRegion2X->setEnabled(checked);
    ui->spNorRegion2BlockCount->setEnabled(checked);
    ui->lblNorRegion2Size->setEnabled(checked);

    if (!checked)
        ui->cboxNorRegion3->setChecked(false);
    ui->cboxNorRegion3->setEnabled(checked);

    updateEraseBlockRegion(norArgs.regions.region[1], ui->spNorRegion2BlockSize, ui->spNorRegion2BlockCount, ui->lblNorRegion2Size);
    updateEraseBlockRegions();
    updateCustomRange();
}

void MainWindow::on_cboxNorRegion3_toggled(bool checked)
{
    ui->spNorRegion3BlockSize->setEnabled(checked);
    ui->lblNorRegion3X->setEnabled(checked);
    ui->spNorRegion3BlockCount->setEnabled(checked);
    ui->lblNorRegion3Size->setEnabled(checked);

    if (!checked)
        ui->cboxNorRegion4->setChecked(false);
    ui->cboxNorRegion4->setEnabled(checked);

    updateEraseBlockRegion(norArgs.regions.region[2], ui->spNorRegion3BlockSize, ui->spNorRegion3BlockCount, ui->lblNorRegion3Size);
    updateEraseBlockRegions();
    updateCustomRange();
}

void MainWindow::on_cboxNorRegion4_toggled(bool checked)
{
    ui->spNorRegion4BlockSize->setEnabled(checked);
    ui->lblNorRegion4X->setEnabled(checked);
    ui->spNorRegion4BlockCount->setEnabled(checked);
    ui->lblNorRegion4Size->setEnabled(checked);

    updateEraseBlockRegion(norArgs.regions.region[3], ui->spNorRegion4BlockSize, ui->spNorRegion4BlockCount, ui->lblNorRegion4Size);
    updateEraseBlockRegions();
    updateCustomRange();
}

void MainWindow::on_spNorRegion1BlockSize_valueChanged(int)
{
    updateEraseBlockRegion(norArgs.regions.region[0], ui->spNorRegion1BlockSize, ui->spNorRegion1BlockCount, ui->lblNorRegion1Size);
    updateCustomRange();
}

void MainWindow::on_spNorRegion2BlockSize_valueChanged(int)
{
    updateEraseBlockRegion(norArgs.regions.region[1], ui->spNorRegion2BlockSize, ui->spNorRegion2BlockCount, ui->lblNorRegion2Size);
    updateCustomRange();
}

void MainWindow::on_spNorRegion3BlockSize_valueChanged(int)
{
    updateEraseBlockRegion(norArgs.regions.region[2], ui->spNorRegion3BlockSize, ui->spNorRegion3BlockCount, ui->lblNorRegion3Size);
    updateCustomRange();
}

void MainWindow::on_spNorRegion4BlockSize_valueChanged(int)
{
    updateEraseBlockRegion(norArgs.regions.region[3], ui->spNorRegion4BlockSize, ui->spNorRegion4BlockCount, ui->lblNorRegion4Size);
    updateCustomRange();
}

void MainWindow::on_spNorRegion1BlockCount_valueChanged(int)
{
    updateEraseBlockRegion(norArgs.regions.region[0], ui->spNorRegion1BlockSize, ui->spNorRegion1BlockCount, ui->lblNorRegion1Size);
    updateCustomRange();
}

void MainWindow::on_spNorRegion2BlockCount_valueChanged(int)
{
    updateEraseBlockRegion(norArgs.regions.region[1], ui->spNorRegion2BlockSize, ui->spNorRegion2BlockCount, ui->lblNorRegion2Size);
    updateCustomRange();
}

void MainWindow::on_spNorRegion3BlockCount_valueChanged(int)
{
    updateEraseBlockRegion(norArgs.regions.region[2], ui->spNorRegion3BlockSize, ui->spNorRegion3BlockCount, ui->lblNorRegion3Size);
    updateCustomRange();
}

void MainWindow::on_spNorRegion4BlockCount_valueChanged(int)
{
    updateEraseBlockRegion(norArgs.regions.region[3], ui->spNorRegion4BlockSize, ui->spNorRegion4BlockCount, ui->lblNorRegion4Size);
    updateCustomRange();
}

void MainWindow::on_cboxNorRangeStart_toggled(bool checked)
{
    ui->spNorRangeStart->setEnabled(checked);
    norArgs.range.startEnabled = checked;

    updateCustomRange();
}

void MainWindow::on_cboxNorRangeEnd_toggled(bool checked)
{
    ui->spNorRangeEnd->setEnabled(checked);
    norArgs.range.endEnabled = checked;

    updateCustomRange();
}

void MainWindow::on_cboxNorRangeSync_toggled(bool checked)
{
    norArgs.range.sync = checked;
}

void MainWindow::on_spNorRangeStart_valueChanged(int)
{
    norArgs.range.start = ui->spNorRangeStart->value();
    updateCustomRange();
}

void MainWindow::on_spNorRangeEnd_valueChanged(int)
{
    norArgs.range.end = ui->spNorRangeEnd->value();
    updateCustomRange();
}

void MainWindow::on_spNorBufferedMaxBytes_valueChanged(int arg1)
{
    norArgs.bufferedMax = 1 << arg1;
}

//void MainWindow::on_cboxNand1CE_A_toggled(bool checked)
//{
//    dualNandArgs.nand[0].CE_A = checked;
//    ui->cboxNand1CE_B->setChecked(!checked);
//}

//void MainWindow::on_cboxNand1CE_B_toggled(bool checked)
//{
//    dualNandArgs.nand[0].CE_B = checked;
//    ui->cboxNand1CE_A->setChecked(!checked);
//}

//void MainWindow::on_cboxNand2CE_A_toggled(bool checked)
//{
//    dualNandArgs.nand[1].CE_A = checked;
//    ui->cboxNand2CE_B->setChecked(!checked);
//}

//void MainWindow::on_cboxNand2CE_B_toggled(bool checked)
//{
//    dualNandArgs.nand[1].CE_B = checked;
//    ui->cboxNand2CE_A->setChecked(!checked);
//}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    switch(index) {
        case 1:
            if(!warnedresistornor)
                QMessageBox::warning(this, tr("WARNING"),
                                      tr("The operation of NOR memories was selected.\nOn ProgSkeet 1.2 and 1.21, R8 must be soldered and R7 must be left open.\nIn addition, proper power supply must be provided to the memory.\nPlease make sure this is the case."));
            warnedresistornor = true;
            break;
    case 2:
            if(!warnedresistornand)
                QMessageBox::warning(this, tr("WARNING"),
                                      tr("The operation of NAND memories was selected.\nOn ProgSkeet 1.2 and 1.21, R7 must be soldered and R8 must be left open.\nIn addition, proper power supply must be provided to the memory.\nPlease make sure this is the case."));
            warnedresistornand = true;
            break;
    default:
        break;
    }

}

void MainWindow::DecodeCFI(const QByteArray& cfi, NorArgs *norargs)
{
    const char* cfip = cfi.data();

    float vcc_supply_min, vcc_supply_max, vpp_supply_min, vpp_supply_max;
    uint32_t timeout_per_word_program;
    uint32_t timeout_minimum_size_write_buffer;
    uint32_t timeout_per_block_erase;
    uint32_t timeout_fullchip_erase;
    uint32_t maximum_timeout_per_word_program;
    uint32_t maximum_timeout_write_buffer;
    uint32_t maximum_timeout_per_block_erase;
    uint32_t maximum_timeout_fullchip_erase;
    uint32_t device_size;
    uint32_t maximum_number_of_bytes;
    uint8_t flash_device_interface;
    uint32_t number_of_erase_block_regions;
    uint32_t region_erase_block[3];
    uint32_t region_block_size[3];

    if (cfip[0] != 'Q' || cfi[1] != 'R' || cfi[2] != 'Y') {
        //statusTextAdded(tr("CFI is invalid (does not begin with \"QRY\")"));
    } else {
        //statusTextAdded(tr("Decoding CFI"));


        norargs->chipCount = 1<<cfip[0x100];

        vcc_supply_min = (float)((cfip[0x0B] & 0xF0) / 16 )+ (float)(cfi[0x0B] & 0x0F)/10;
        vcc_supply_max = (float)((cfip[0x0C] & 0xF0) / 16 )+ (float)(cfi[0x0C] & 0x0F)/10;
        vpp_supply_min = (float)((cfip[0x0D] & 0xF0) / 16 )+ (float)(cfi[0x0D] & 0x0F)/10;
        vpp_supply_max = (float)((cfip[0x0E] & 0xF0) / 16 )+ (float)(cfi[0x0E] & 0x0F)/10;

        timeout_per_word_program = 1<<cfip[0x0F];
        timeout_minimum_size_write_buffer = cfip[0x10]?1<<cfip[0x10]:0;
        timeout_per_block_erase = 1<<cfip[0x11];
        timeout_fullchip_erase = cfip[0x12]?1<<cfip[0x12]:0;
        maximum_timeout_per_word_program = timeout_per_word_program * 1<<cfip[0x13];
        maximum_timeout_write_buffer = timeout_minimum_size_write_buffer * 1<<cfip[0x14];
        maximum_timeout_per_block_erase = timeout_per_block_erase * 1<<cfip[0x15];
        maximum_timeout_fullchip_erase = timeout_fullchip_erase * 1<<cfip[0x16];

        device_size = 1<<cfip[0x17];
        norargs->deviceSize = device_size;

        maximum_number_of_bytes = 1<<cfip[0x1A];
        norargs->bufferedMax = maximum_number_of_bytes;

        flash_device_interface = cfip[0x18];

        number_of_erase_block_regions = cfip[0x1C];
        norargs->regions.count = number_of_erase_block_regions;

//        statusTextAdded(tr(" - VCC = %1V - %2V").arg(vcc_supply_min).arg(vcc_supply_max));
//        if (vpp_supply_min > 0.0)
//            statusTextAdded(tr(" - VPP = %1V - %2V").arg(vpp_supply_min).arg(vpp_supply_max));
//        else
//            statusTextAdded(tr(" - VPP = NA"));

//        statusTextAdded(tr(" - Timeout per word program = %1us").arg(timeout_per_word_program));
        norargs->timings.tWHWH1_Word = timeout_per_word_program;

//        if (timeout_minimum_size_write_buffer)
//            statusTextAdded(tr(" - Timeout for minimum buffer write = %1us").arg(timeout_minimum_size_write_buffer));
//        else
//            statusTextAdded(tr(" - Timeout for minimum buffer write = NA"));
        norargs->timings.tWHWH1_BufWord = timeout_minimum_size_write_buffer;


//        statusTextAdded(tr(" - Timeout per individual block erase = %1ms").arg(timeout_per_block_erase));

//        if (timeout_fullchip_erase)
//            statusTextAdded(tr(" - Timeout for full chip erase = %1ms").arg(timeout_fullchip_erase));
//        else
//            statusTextAdded(tr(" - Timeout for full chip erase = NA"));

//        statusTextAdded(tr(" - Maximum timeout per word program = %1us").arg(maximum_timeout_per_word_program));

//        if (maximum_timeout_write_buffer)
//            statusTextAdded(tr(" - Maximum timeout for write buffer = %1us").arg(maximum_timeout_write_buffer));
//        else
//            statusTextAdded(tr(" - Maximum timeout for write buffer = NA"));

//        statusTextAdded(tr(" - Maximum timeout per block erase = %1s").arg(maximum_timeout_per_block_erase/1000));

//        if (maximum_timeout_fullchip_erase)
//            statusTextAdded(tr(" - Maximum timeout for full chip erase %1ms").arg(maximum_timeout_fullchip_erase));
//        else
//            statusTextAdded(tr(" - Maximum timeout for full chip erase = NA"));

//        statusTextAdded(tr(" - Device size = %1MiB (%2)").arg(device_size/1024/1024).arg(device_size));

//        statusTextAdded(tr(" - Maximum number of bytes for buffered write = %1").arg(maximum_number_of_bytes));

//        statusTextAdded(tr(" - Flash device interface code description = %1").arg(
//                            flash_device_interface == 0 ? tr("8bits only") :
//                                                          (flash_device_interface == 1 ?
//                                                               tr("16bits only") :
//                                                               tr("8bits/16bits"))));

//        statusTextAdded(tr(" - Number of erase block regions = %1").arg(number_of_erase_block_regions));

        for(uint8_t r=0; r < number_of_erase_block_regions; r++) {
//            statusTextAdded(tr(" - Region %1").arg(r+1));

            region_erase_block[r] = (uint32_t)cfip[0x1D+(r<<2)]+1;
            region_block_size[r] = ( (cfip[0x20+(r<<2)]<<8) + (uint32_t)cfip[0x1f + (r<<2)]) << 8;
//            statusTextAdded(tr("    - Region %1 number of erase blocks = %2").arg(r+1).arg(region_erase_block[r]));

            norargs->regions.region[r].blockCount = region_erase_block[r];
            norargs->regions.region[r].blockSize = region_block_size[r];

//            if (region_block_size[r] > 1024-1) {
//                if (region_block_size[r] > 1024*1024-1)
//                    statusTextAdded(tr("    - Region %1 block size = %2Mbyte").arg(r+1).arg(region_block_size[r]/1024/1024));
//                else
//                    statusTextAdded(tr("    - Region %1 block size = %2Kbyte").arg(r+1).arg(region_block_size[r]/1024));
//            } else {
//                statusTextAdded(tr("    - Region %1 block size = %2 bytes").arg(r+1).arg(region_block_size[r]));
//            }
        }
    }
}


void MainWindow::on_cbNorPresets_currentIndexChanged(int index)
{
    NorArgs norargstemp;

    QFile fp("./nor/"+ui->cbNorPresets->itemText(index));
    if(fp.open(QIODevice::ReadOnly))
    {
        QByteArray fileBuf = fp.read(0x102);
        fp.close();
        norargstemp.bufferedMax = 0;
        norargstemp.chipCount = 0;
        norargstemp.deviceSize = 0;
        norargstemp.regions.count = 0;
        for(int i=0; i<4; i++) {
            norargstemp.regions.region[i].blockCount = 0;
            norargstemp.regions.region[i].blockSize = 0;
        }

        DecodeCFI(fileBuf, &norargstemp);
        norArgs.timings = norargstemp.timings;
        ui->spNorDeviceSize->setValue(logbin(norargstemp.deviceSize));




            ui->spNorRegion1BlockCount->setValue(norargstemp.regions.region[0].blockCount);
            ui->spNorRegion1BlockSize->setValue(norargstemp.regions.region[0].blockSize);

            ui->spNorRegion2BlockCount->setValue(norargstemp.regions.region[1].blockCount);
            ui->spNorRegion2BlockSize->setValue(norargstemp.regions.region[1].blockSize);

            ui->spNorRegion3BlockCount->setValue(norargstemp.regions.region[2].blockCount);
            ui->spNorRegion3BlockSize->setValue(norargstemp.regions.region[2].blockSize);

            ui->spNorRegion4BlockCount->setValue(norargstemp.regions.region[3].blockCount);
            ui->spNorRegion4BlockSize->setValue(norargstemp.regions.region[3].blockSize);

            ui->cboxNorRegion2->setChecked((norargstemp.regions.count > 1)?true:false);
            ui->cboxNorRegion3->setChecked((norargstemp.regions.count > 2)?true:false);
            ui->cboxNorRegion4->setChecked((norargstemp.regions.count > 3)?true:false);



            if(norargstemp.bufferedMax <2)
                ui->cbNorWriteMethod->setCurrentIndex((int)NorWriteMethod_SingleWordProgram);
            else
            {
                ui->cbNorWriteMethod->setCurrentIndex((int)NorWriteMethod_BufferedWrite);
                ui->spNorBufferedMaxBytes->setValue(logbin(norargstemp.bufferedMax));
            }

            ui->spNorChipCount->setValue(norargstemp.chipCount);
    }


}


void MainWindow::NorPresetLoad()
{
    ui->cbNorPresets->clear();
    QDir dir("./nor/");

    dir.setNameFilters(QStringList("*"));
      dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

      //qDebug() << "Scanning: " << dir.path();

      QStringList fileList = dir.entryList();
      for(int i=0; i<fileList.count(); i++)
          ui->cbNorPresets->addItem(fileList[i]);
      ui->cbNorPresets->setCurrentIndex(0);
}

void MainWindow::NandPresetLoad()
{
    ui->cbNandPresets->clear();
    QDir dir("./nand/");

    dir.setNameFilters(QStringList("*"));
      dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

      //qDebug() << "Scanning: " << dir.path();

      QStringList fileList = dir.entryList();
      for(int i=0; i<fileList.count(); i++)
          ui->cbNandPresets->addItem(fileList[i]);
      ui->cbNandPresets->setCurrentIndex(0);
}

void MainWindow::on_cbNandPresets_currentIndexChanged(int index)
{
    QFile fp("./nand/"+ui->cbNorPresets->itemText(index));
    if(fp.open(QIODevice::ReadOnly))
    {
        QByteArray fileBuf = fp.read(16);
        fp.close();
    }
}

void MainWindow::on_pushButton_clicked()
{
    if (!::CreateDevice()) {
        QMessageBox::critical(this, tr("%1 read id information").arg(TARGETNAME),
                              tr("Device not found!"));
        return;
    }

    uint8_t buffer[256];
    uint16_t id =0;

    memset(buffer, 0, 256);
    ::SF_ReadID(buffer);
    ::SF_Read(0, 256, buffer);
    ::RxStart();

    ::RemoveDevice();


}

void MainWindow::on_cboxNand1Primary_toggled(bool checked)
{
    dualNandArgs.nand[0].enabled[0] = checked;
}

void MainWindow::on_cboxNand1Secondary_toggled(bool checked)
{
    dualNandArgs.nand[0].enabled[1] = checked;
}

void MainWindow::on_cboxNand2Primary_toggled(bool checked)
{
    dualNandArgs.nand[1].enabled[0] = checked;
}

void MainWindow::on_cboxNand2Secondary_toggled(bool checked)
{
    dualNandArgs.nand[1].enabled[1] = checked;
}

void MainWindow::on_cbNandPagesPerBlock_currentIndexChanged(int index)
{
    dualNandArgs.Settings.pageCount = 1<<index;
}



void MainWindow::on_cbNandBlockCount_currentIndexChanged(int index)
{
    dualNandArgs.Settings.blockCount = 1<<index;

    ui->cboxNandCustomRange->setChecked(false);

    dualNandArgs.range.start = 0;
    dualNandArgs.range.end = (1 << index) - 1;

}

void MainWindow::on_cboxNandBigBlock_toggled(bool checked)
{
    dualNandArgs.Settings.bigBlock = checked;
}



void MainWindow::on_cboxNandRaw_toggled(bool checked)
{
    dualNandArgs.Settings.raw = checked;
}

//void MainWindow::on_cboxNandCustomRange_toggled(bool checked)
//{
//    ui->frmNandCustomRange->setEnabled(checked);

//    if (checked) {
//        ui->cbNandBlockStart->clear();
//        ui->cbNandBlockEnd->clear();

//        QString text;
//        int curIdx = ui->cbNandBlockCount->currentIndex();
//        for (int i = 0; i < (1 << curIdx); i++) {
//            text = QString::number(i);
//            ui->cbNandBlockStart->addItem(text);
//            ui->cbNandBlockEnd->addItem(text);
//        }

//        ui->cbNandBlockStart->setCurrentIndex(0);
//        ui->cbNandBlockEnd->setCurrentIndex(ui->cbNandBlockEnd->count() - 1);
//    }
//}

void MainWindow::on_cbNandBlockStart_currentIndexChanged(int index)
{
    if (index > ui->cbNandBlockEnd->currentIndex())
        ui->cbNandBlockEnd->setCurrentIndex(index);

    dualNandArgs.range.start = index;
}

void MainWindow::on_cbNandBlockEnd_currentIndexChanged(int index)
{
    if (index < ui->cbNandBlockStart->currentIndex())
        ui->cbNandBlockStart->setCurrentIndex(index);

    dualNandArgs.range.end = index;
}

void MainWindow::on_cbNandPageSize_currentIndexChanged(int index)
{
    dualNandArgs.Settings.bytesPerPage = (1<<index)*512;
}

void MainWindow::on_cbNandAddressCycles_currentIndexChanged(int index)
{
    dualNandArgs.Settings.addresscycles = index+3;
}

//void MainWindow::on_cbNandAction_currentIndexChanged(int index)
//{
//    ui->cbNandAction->itemData(index).toInt();
//}

//void MainWindow::on_horizontalSlider_valueChanged(int value)
//{

//    ui->lblGlitchOffset->setText(QString::number(value));
//}

void MainWindow::on_pushButton_5_clicked()
{
    uint16_t start, len;

    if (!::CreateDevice()) {
        QMessageBox::critical(this, tr("%1 Glitch information").arg(TARGETNAME),
                              tr("Device not found!."));
        return;
    }

    start = ui->spGlitchStart->value();
    len = ui->spGlitchLength->value();

    while(1)
    {
        addStatusText(QString("Trying: Start = %1 , Length = %2").arg(start).arg(len));
        QCoreApplication::processEvents();
        ::Glitch_Set(start, start+len);
        start++;
    }

    ::RemoveDevice();
}

void MainWindow::on_btnCancel_clicked()
{

}
