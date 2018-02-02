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
