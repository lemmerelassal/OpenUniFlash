#include "sizespinbox.h"

SizeSpinBox::SizeSpinBox(QWidget* parent) :
    QSpinBox(parent)
{
}

#define SIZE_KIB 1024ull
#define SIZE_MIB (SIZE_KIB * 1024ull)
#define SIZE_GIB (SIZE_MIB * 1024ull)
#define SIZE_TIB (SIZE_GIB * 1024ull)
#define SIZE_PIB (SIZE_TIB * 1024ull)

#define SIZE_KB 1000ull
#define SIZE_MB (SIZE_KB * 1000ull)
#define SIZE_GB (SIZE_MB * 1000ull)
#define SIZE_TB (SIZE_GB * 1000ull)
#define SIZE_PB (SIZE_TB * 1000ull)

QString SizeSpinBox::formatSize(uint64_t value)
{
    /* Value is in bytes */

    if (value == 1) {
        return tr("1 byte");
    } else if (value < SIZE_KIB || (value % SIZE_KIB) > 0) {
        return tr("%1 bytes").arg(value);
    } else if (value < SIZE_MIB || (value % SIZE_MIB) > 0) {
        return tr("%1 KiB").arg(value / SIZE_KIB);
    } else if (value < SIZE_GIB || (value % SIZE_GIB) > 0) {
        return tr("%1 MiB").arg(value / SIZE_MIB);
    } else if (value < SIZE_TIB || (value % SIZE_TIB) > 0) {
        return tr("%1 GiB").arg(value / SIZE_GIB);
    } else if (value < SIZE_PIB || (value % SIZE_PIB) > 0) {
        return tr("%1 TiB").arg(value / SIZE_TIB);
    }

    /* PiB */
    return tr("%1 PiB").arg(value / SIZE_PIB);
}

uint64_t SizeSpinBox::scanSize(const QString& text)
{
    QString trimmed = text.trimmed();
    QRegExp rx("^([0-9]*)(.*)$");
    if (rx.indexIn(trimmed) == -1)
        return text.toLongLong();

    QString num = rx.cap(1);
    QString unit = rx.cap(2).trimmed();

    if (unit.compare(tr("byte"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong();
    } else if (unit.compare(tr("bytes"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong();
    } else if (unit.compare(tr("KiB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_KIB;
    } else if (unit.compare(tr("MiB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_MIB;
    } else if (unit.compare(tr("GiB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_GIB;
    } else if (unit.compare(tr("TiB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_TIB;
    } else if (unit.compare(tr("PiB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_PIB;
    } else if (unit.compare(tr("KB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_KB;
    } else if (unit.compare(tr("MB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_MB;
    } else if (unit.compare(tr("GB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_GB;
    } else if (unit.compare(tr("TB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_TB;
    } else if (unit.compare(tr("PB"), Qt::CaseInsensitive) == 0) {
        return num.toLongLong() * SIZE_PB;
    }

    return text.toLongLong();
}

QString SizeSpinBox::textFromValue(int value) const
{
    return formatSize(value);
}

int SizeSpinBox::valueFromText(const QString& text) const
{
    return scanSize(text);
}

QValidator::State SizeSpinBox::validate(QString&, int&) const
{
    return QValidator::Acceptable;
}
