/*
OpenUniFlash - A universal NAND and NOR Flash programmer
Copyright (C) 2010-2018  Lemmer EL ASSAL, Axel GEMBE, Maël Blocteur

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
Also add information on how to contact you by electronic and paper mail.
*/

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
