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

#include <stdint.h>
#include <QString>

struct CommonArgs
{
    uint32_t word;
    uint32_t differential;
    uint32_t verify;
    uint32_t byteSwap;
    uint32_t abortOnError;
    uint8_t period;
};

struct WriteLocation {
    uint32_t base; /* base address of the chip, important for multichip devices; includes A0 */
    uint32_t saddr; /* sector address */
    uint32_t offset; /* offset of the location */
    uint32_t length; /* length in words */
    uint8_t *data;
    WriteLocation *_next;
};


struct RangeArgs
{
    bool startEnabled;
    uint32_t start;

    bool endEnabled;
    uint32_t end;

    bool sync;
};

struct EraseBlockRegion
{
    uint32_t blockSize;
    uint32_t blockCount;
};

struct EraseBlockRegions
{
    uint32_t count;
    struct EraseBlockRegion region[4];
};

enum NorWriteMethod
{
    NorWriteMethod_BufferedWrite = 0,
    NorWriteMethod_SingleWordProgram,
    NorWriteMethod_DoubleWordProgram
};

enum NorWaitMethod
{
    NorWaitMethod_RdyTrigger = 0,
    NorWaitMethod_DataPolling,
    NorWaitMethod_Static,
    NorWaitMethod_TxStart
};

struct NorTimings
{
    /* Buffered write time for a single word in us */
    uint32_t tWHWH1_BufWord;

    /* Single word write time in us */
    uint32_t tWHWH1_Word;

    /* RY/BY# recovery time in ns*/
    uint32_t tRB;

    /* Erase / program valid to RY/BY# delay in ns */
    uint32_t tBUSY;

};

struct NorArgs
{
    uint32_t deviceSize;
    uint32_t chipCount;

    struct EraseBlockRegions regions;

    struct RangeArgs range;

    struct NorTimings timings;

    enum NorWriteMethod writeMethod;
    enum NorWaitMethod waitMethod;

    uint32_t bufferedMax;
};

struct NandPins {
    uint8_t NAND_CE_A;
    uint8_t NAND_CE_B;
    uint8_t NAND_CLE;
    uint8_t NAND_ALE;
    uint8_t NAND_WP;
    uint8_t NAND_RB;
    uint8_t NAND_WE;
    uint8_t NAND_RE;
};

struct NandSettings {
    uint32_t pageCount;
    uint32_t blockCount;
    uint32_t bigBlock;
    uint32_t raw;
    uint32_t bytesPerPage;
    uint32_t bytesPerBlock;
    uint32_t read_page_delay_us;
    uint8_t addresscycles;
};

struct NandArgs
{
    bool enabled[2];
    char * buffer[2];
    struct NandPins Pins;
};

struct DualNandArgs
{
    struct NandArgs nand[2];
    struct NandSettings Settings;
    struct RangeArgs range;
};

struct Nand
{
	char	 maker_code;
	char 	 device_id;
	uint32_t internal_chip_number;
        uint32_t cell_type;
        uint32_t num_of_simult_prg_pages;
        uint32_t interleave_program;
        uint32_t cache_program;

        uint32_t page_size;
        uint32_t spare_512;
        uint32_t block_size;

        uint32_t plane;
        uint32_t plane_size;
	uint32_t block_count;
};
