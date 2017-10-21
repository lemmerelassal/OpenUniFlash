#pragma once

#include "sizespinbox.h"

class EraseBlockSpinBox : public SizeSpinBox
{
    Q_OBJECT
public:
    explicit EraseBlockSpinBox(QWidget* parent = 0);

    void setEraseBlockRegions(const struct EraseBlockRegions* ebrs);
    const struct EraseBlockRegions* eraseBlockRegions() const;

    void setExtraBlock(bool enabled);
    bool extraBlock() const;

    void setDeviceSize(const int deviceSize);
    int deviceSize() const;

    void setChipCount(const int chipCount);
    int chipCount() const;

    virtual void stepBy(int steps);

private:
    const struct EraseBlockRegions* ebrs;
    bool m_extraBlock;
    int m_deviceSize;
    int m_chipCount;
};
