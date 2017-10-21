#pragma once

#include "sizespinbox.h"

class BinSizeSpinBox : public SizeSpinBox
{
    Q_OBJECT
public:
    explicit BinSizeSpinBox(QWidget* parent = 0);

protected:
    virtual QString textFromValue(int value) const;
    virtual int valueFromText(const QString& text) const;
};
