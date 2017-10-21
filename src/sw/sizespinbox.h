#pragma once

#include <stdint.h>
#include <QSpinBox>

class SizeSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit SizeSpinBox(QWidget* parent = 0);

    static QString formatSize(uint64_t value);
    static uint64_t scanSize(const QString& text);

protected:
    virtual QString textFromValue(int value) const;
    virtual int valueFromText(const QString& text) const;
    virtual QValidator::State validate(QString& text, int& pos) const;
};
