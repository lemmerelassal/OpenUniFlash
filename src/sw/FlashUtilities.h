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
