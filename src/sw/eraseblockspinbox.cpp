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

#include "eraseblockspinbox.h"
#include "FlashUtilities.h"

EraseBlockSpinBox::EraseBlockSpinBox(QWidget* parent) :
    SizeSpinBox(parent)
{
    ebrs = NULL;
    m_extraBlock = false;
    m_chipCount = 1;
    m_deviceSize = 1;
}

void EraseBlockSpinBox::setEraseBlockRegions(const struct EraseBlockRegions* ebrs)
{
    this->ebrs = ebrs;
}

const struct EraseBlockRegions* EraseBlockSpinBox::eraseBlockRegions() const
{
    return ebrs;
}

void EraseBlockSpinBox::stepBy(int steps)
{
    /* No sir, I don't like it */
    if (steps == 0 || ebrs == NULL)
        return;

    uint32_t chipSize = 1 << m_deviceSize;
    uint32_t addr = value() % chipSize;
    int chip = value() / chipSize;

    while (steps > 0 && chip < m_chipCount) {
        /* Step up */
        uint32_t next = FlashUtilities::NextEBStart(ebrs, addr);

        /* After end of chip? */
        if ((next / (chipSize - 1)) > 0) {
            /*
             * If chips are left or the extra block is enabled,
             * use next and continue from address 0
             */
            if (chip + 1 < m_chipCount || m_extraBlock) {
                chip += 1;
                addr = 0;
                break;
            }

            break;
        }

        addr = next;

        steps--;
    }

    while (steps < 0 && chip >= 0) {
        /* Step down */
        uint32_t prev = FlashUtilities::PrevEBStart(ebrs, addr);

        /* Before beginning of chip? */
        if (prev == (uint32_t)~0) {
            /* If chips are left, use previous and continue from address ~0 */
            if (--chip >= 0) {
                addr = (uint32_t)~0;
                continue;
            }

            /* No chips left, stop stepping */
            break;
        }

        addr = prev;

        steps++;
    }

    setValue((chip * chipSize) + addr);
}

void EraseBlockSpinBox::setExtraBlock(bool enabled)
{
    m_extraBlock = enabled;
}

bool EraseBlockSpinBox::extraBlock() const
{
    return m_extraBlock;
}

void EraseBlockSpinBox::setDeviceSize(const int deviceSize)
{
    m_deviceSize = deviceSize;
}

int EraseBlockSpinBox::deviceSize() const
{
    return m_deviceSize;
}

void EraseBlockSpinBox::setChipCount(const int chipCount)
{
    m_chipCount = chipCount;
}

int EraseBlockSpinBox::chipCount() const
{
    return m_chipCount;
}
