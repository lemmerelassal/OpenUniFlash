#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>

#include "Arguments.h"
#include "FlasherThread.h"

#include "binsizespinbox.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    /* Flash thread common slots */
    void setProgressRange(const int min, const int max);
    void setProgress(const int cur);
    void addStatusText(const QString& text);
    void onFinished();
    void UpdateRxSpeed(const int value);
    void UpdateTxSpeed(const int value);
    void setProgressText(const QString& text);


private slots:
    /* General */
    void on_btnClear_clicked();

    /* Common - Options */
    void on_cboxDifferential_toggled(bool checked);
    void on_cboxVerify_toggled(bool checked);
    void on_cboxAbortOnError_clicked(bool checked);
    void on_cboxByteSwap_toggled(bool checked);
    void on_cboxDouble_toggled(bool checked);
    void on_cbPeriod_currentIndexChanged(int index);
    void on_inputReadDelay_textChanged(QString arg1);

    /* Common - Presets */
    void on_cbPresets_currentIndexChanged(int index);
    void on_btnPresetNew_clicked();
    void on_btnPresetSave_clicked();
    void on_btnPresetDelete_clicked();

    /* NOR - Buttons */
    void on_btnNorDump_clicked();
    void on_btnNorFlash_clicked();
    void on_btnNorErase_clicked();
    void on_btnNorDumpCFI_clicked();
    void on_btnNorDetect_clicked();

    /* NOR - Device Size */
    void on_spNorDeviceSize_valueChanged(int arg1);
    void on_spNorChipCount_valueChanged(int arg1);

    /* NOR - Erase block region 1 */
    void on_spNorRegion1BlockSize_valueChanged(int arg1);
    void on_spNorRegion1BlockCount_valueChanged(int arg1);

    /* NOR - Erase block region 2 */
    void on_cboxNorRegion2_toggled(bool checked);
    void on_spNorRegion2BlockSize_valueChanged(int arg1);
    void on_spNorRegion2BlockCount_valueChanged(int arg1);

    /* NOR - Erase block region 3 */
    void on_cboxNorRegion3_toggled(bool checked);
    void on_spNorRegion3BlockSize_valueChanged(int arg1);
    void on_spNorRegion3BlockCount_valueChanged(int arg1);

    /* NOR - Erase block region 4 */
    void on_cboxNorRegion4_toggled(bool checked);
    void on_spNorRegion4BlockSize_valueChanged(int arg1);
    void on_spNorRegion4BlockCount_valueChanged(int arg1);

    /* NOR - Custom Range */
    void on_cboxNorRangeStart_toggled(bool checked);
    void on_spNorRangeStart_valueChanged(int arg1);
    void on_cboxNorRangeEnd_toggled(bool checked);
    void on_spNorRangeEnd_valueChanged(int arg1);
    void on_cboxNorRangeSync_toggled(bool checked);

    /* NOR - Options */
    void on_cbNorWriteMethod_currentIndexChanged(int index);
    void on_spNorBufferedMaxBytes_valueChanged(int arg1);
    void on_cbNorWaitMethod_currentIndexChanged(int index);

    /* NAND - Buttons */
    void on_btnNandDump_clicked();
    void on_btnNandFlash_clicked();
    void on_btnNandErase_clicked();

    /* NAND 1 */
    void on_cboxNand1_toggled(bool checked);
    void on_cbNand1PagesPerBlock_currentIndexChanged(int index);
    void on_cbNand1BlockCount_currentIndexChanged(int index);
    //void on_cboxNand1CustomRange_toggled(bool checked);
    void on_cbNand1BlockStart_currentIndexChanged(int index);
    void on_cbNand1BlockEnd_currentIndexChanged(int index);
    void on_cboxNand1BigBlock_toggled(bool checked);
    void on_cboxNand1Raw_toggled(bool checked);
    void on_btnAutoNand1_clicked();
    void on_btnAutoNand2_clicked();

    /* NAND 2 */
    void on_cboxNand2_toggled(bool checked);
    //void on_cbNand2PagesPerBlock_currentIndexChanged(int index);
    void on_cbNand2BlockCount_currentIndexChanged(int index);
    void on_cboxNand2CustomRange_toggled(bool checked);
    void on_cbNand2BlockStart_currentIndexChanged(int index);
    void on_cbNand2BlockEnd_currentIndexChanged(int index);
    void on_cboxNand2BigBlock_toggled(bool checked);
    void on_cboxNand2Raw_toggled(bool checked);

    /* Patcher */
    void on_btnInputFileBrowse_clicked();
    void on_btnPatchFileBrowse_clicked();
    void on_btnApply_clicked();

    /* Tools */
//    void on_btnTestShorts_clicked();
//    void on_btnSetAddr_clicked();
//    void on_btnSetData_clicked();

//    void on_cboxNand1CE_A_toggled(bool checked);

//    void on_cboxNand1CE_B_toggled(bool checked);

//    void on_cboxNand2CE_A_toggled(bool checked);

//    void on_cboxNand2CE_B_toggled(bool checked);

    void on_tabWidget_currentChanged(int index);

    void on_cbNorPresets_currentIndexChanged(int index);

    void on_cbNandPresets_currentIndexChanged(int index);

    void on_pushButton_clicked();

    void on_cboxNand1Primary_toggled(bool checked);

    void on_cboxNand1Secondary_toggled(bool checked);

    void on_cboxNand2Primary_toggled(bool checked);

    void on_cboxNand2Secondary_toggled(bool checked);

    void on_cbNandPagesPerBlock_currentIndexChanged(int index);

    void on_cbNandBlockCount_currentIndexChanged(int index);

    void on_cboxNandBigBlock_toggled(bool checked);

    void on_cboxNandRaw_toggled(bool checked);

    void on_cboxNandCustomRange_toggled(bool checked);

    void on_cbNandBlockStart_currentIndexChanged(int index);

    void on_cbNandBlockEnd_currentIndexChanged(int index);

    void on_cbNandPageSize_currentIndexChanged(int index);

    void on_cbNandAddressCycles_currentIndexChanged(int index);



//    void on_horizontalSlider_valueChanged(int value);

//    void on_cbNandAction_currentIndexChanged(int index);

    void on_pushButton_5_clicked();

    void on_btnCancel_clicked();

signals:
    void cancelled();

private:
    Ui::MainWindow *ui;

    void connectFlasherThread(FlasherThread* flasherThread);

    void updateTotals(const BinSizeSpinBox* size, const QSpinBox* count, QLabel* label);
    void updateTotals(const QSpinBox* size, const QSpinBox* count, QLabel* label);

    void updateEraseBlockRegion(EraseBlockRegion& ebr, const QSpinBox* size, const QSpinBox* count, QLabel* label);
    void updateEraseBlockRegions();

    void updateCustomRange();

    void disableButtons(bool disabled);

    DualNandArgs getNandArgs();

    void presetsInit();
    int presetIndexNext();
    int presetNew(const QString& name);
    void presetSave(int index);
    void presetLoad(const int index);
    void presetDelete(const int index);

    CommonArgs commonArgs;
    NorArgs norArgs;
    DualNandArgs dualNandArgs;

    QSettings settings;

    void DecodeCFI(const QByteArray& cfi, NorArgs *norargs);
    void NorPresetLoad();
    void NandPresetLoad();
};


#endif // MAINWINDOW_H
