#include "binsizespinbox.h"

BinSizeSpinBox::BinSizeSpinBox(QWidget* parent) :
    SizeSpinBox(parent)
{
}

QString BinSizeSpinBox::textFromValue(int value) const
{
    return formatSize(1ull << (uint64_t)value);
}

int BinSizeSpinBox::valueFromText(const QString& text) const
{
    uint64_t size = scanSize(text);
    uint64_t shift = 0;

    while (size >>= 1)
        shift++;

    return shift;
}
