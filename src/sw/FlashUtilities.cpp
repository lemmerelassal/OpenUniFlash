#include "FlashUtilities.h"

namespace FlashUtilities
{
    uint32_t EBRSize(const EraseBlockRegion* ebr)
    {
        return (ebr->blockSize * ebr->blockCount);
    }

    uint32_t EBROffset(const EraseBlockRegions* ebrs, uint32_t ebr)
    {
        uint32_t ret = 0;

        for (uint32_t i = 0; i < ebr && i < ebrs->count; i++)
            ret += EBRSize(ebrs->region + i);

        return ret;
    }

    int EBRFind(const EraseBlockRegions* ebrs, uint32_t adr)
    {
        uint32_t offset = 0;
        uint32_t i;

        for (i = 0; i < ebrs->count; i++) {
            uint32_t end = EBRSize(ebrs->region + i) + offset;

            if (adr < end)
                return i;

            offset = end;
        }

        return -1;
    }

    uint32_t NextEBStart(const EraseBlockRegions* ebrs, uint32_t curAdr)
    {
        int ebr = EBRFind(ebrs, curAdr);
        if (ebr < 0)
            ebr = ebrs->count - 1;

        /* Found the EBR that curAdr is in, build address relative to start of EBR */
        uint32_t offset = EBROffset(ebrs, ebr);

        if (ebrs->region[ebr].blockSize == 0)
            return ~0;

        /* Find current block in EBR by dividing by block size */
        uint32_t block = (curAdr - offset) / ebrs->region[ebr].blockSize;

        /* We want the next block */
        return offset + (++block * ebrs->region[ebr].blockSize);
    }

    uint32_t PrevEBStart(const EraseBlockRegions* ebrs, uint32_t curAdr)
    {
        int ebr = EBRFind(ebrs, curAdr);
        if (ebr < 0)
            ebr = ebrs->count - 1;

        /* Found the EBR that curAdr is in, build address relative to start of EBR */
        uint32_t offset = EBROffset(ebrs, ebr);

        /* Find current block in EBR by dividing by block size */
        uint32_t block = (curAdr - offset) / ebrs->region[ebr].blockSize;

        /* Check the previous block is before the start of this region */
        if (block == 0 && ebr == 0)
            return ~0;

        if (block == 0) {
            ebr--;
            block = (ebrs->region[ebr].blockCount - 1);
            return EBROffset(ebrs, ebr) + (block * ebrs->region[ebr].blockSize);
        }

        if (block >= ebrs->region[ebr].blockCount)
            block = ebrs->region[ebr].blockCount;

        /* We want the next block */
        return offset + (--block * ebrs->region[ebr].blockSize);
    }
}
