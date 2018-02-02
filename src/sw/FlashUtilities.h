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

#include "Arguments.h"

namespace FlashUtilities
{
    uint32_t EBRSize(const EraseBlockRegion* ebr);
    uint32_t EBROffset(const EraseBlockRegions* ebrs, uint32_t ebr);
    int EBRFind(const EraseBlockRegions* ebrs, uint32_t adr);
    uint32_t NextEBStart(const EraseBlockRegions* ebrs, uint32_t curAdr);
    uint32_t PrevEBStart(const EraseBlockRegions* ebrs, uint32_t curAdr);
}
