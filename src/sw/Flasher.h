#ifndef __FLASHER_H
#define __FLASHER_H
#include <stdint.h>
#include "Arguments.h"

#ifndef WIN32

/* Linux / OSX */
typedef int32_t DWORD;
typedef int16_t WORD;
typedef char CHAR;
typedef char* PCHAR;
typedef uint8_t BYTE;

#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <stdio.h>

#else

/* WIN32 */

#define use_libusb_win32


#ifdef use_libusb_win32
    #include <lusb0_usb.h>
#else
    #include <libusb.h>
#endif



#endif

#define NOR_GP(x)               (1 << x)

#define NOR_CE			NOR_GP(0)
#define NOR_RST			NOR_GP(1)
#define SB_TRI			NOR_GP(2)
#define NOR_WP			NOR_GP(3)
#define NOR_ALT_RDY		NOR_GP(4)

#define NOR_DEFAULT_DIR         (NOR_RST | NOR_CE | NOR_WP | SB_TRI)
#define NOR_DEFAULT_OUT         (NOR_RST | NOR_WP)


typedef void (*pfnDebugOut)(const char* string);

 //       QString csPath;


        void* mymalloc(uint32_t size);
        void CancelOperations();

	uint8_t CreateDevice();
	void RemoveDevice();

        void EnqueueCommand(uint8_t cmd, uint32_t args, uint8_t argsize,
                            uint8_t *src, uint32_t srcsize,
                            uint8_t *dest, uint32_t destsize,
                            bool critical);
        void GetTimeBase(uint16_t *dest);
        void GetVersion(uint16_t *version);

        void DebugSetup(pfnDebugOut out);
        void DebugWrite(const char* format, ...);

        int TxStart();
        int TxStartThreshold(uint32_t size);
        void Write(uint32_t dwCount, const char * pSrc);
	void WriteToAddress(uint32_t dwAddress, uint16_t wData);
	void ReadFromAddress(uint32_t dwAddress, uint16_t *pwData);

	void SetData(uint16_t wData);

	void Stb(uint8_t bValue, uint8_t bMirror);
	void Sth(uint16_t wValue, uint8_t bMirror);
	void Stw(uint32_t dwValue, uint8_t bMirror);
        int RxStart();
        void Read(uint32_t dwCount, char * pbyDestination);
	void SetDirections(uint16_t directions);
	void SetAddress(uint32_t dwAddress, uint8_t bAutoIncrement);
        void SetOutputs(uint16_t wOutputs);
	void SetConfig(uint8_t byDelay, uint8_t bDouble, uint8_t bTristate, uint8_t bWaitReady, uint8_t bByteSwap);
        void Nop(uint16_t byAmount);
        void SetTimeOut(uint32_t Value); // in ns

	// NOR Flash
        int NOR_Configure(struct CommonArgs common, struct NorArgs nor);
        int NOR_Read(char* buf, uint32_t addr, uint32_t len);
        void NOR_BufferWrite(uint32_t base /* actual chip address on the bus */, uint32_t offset /* actual offset on the bus */, const char* data, uint32_t len /* number of words */);
        void NOR_Program(uint32_t base /* address of chip */, uint32_t offset, const char* data, uint32_t len, const char* currentdata); // addr in bytes, len in bytes
        uint8_t NOR_Erase(uint32_t addr, uint32_t base);
	void NOR_PPBErase();
        int NOR_Reset();
        int NOR_ReadCFI(char* buf, uint32_t len);
void PrechargeRdy();

	// NAND Flash
        void NAND_SelectChip(uint8_t j);
        void NAND_ReadID(char ** pbyDest, uint8_t byCount);
//        void NAND_ReadID(char * pbyDest, uint8_t byCount, uint8_t bChip, uint8_t bPrimary);
	void NAND_Configure(uint16_t wPageSize, uint32_t dwBlockSize, uint32_t dwBlockCount, char bLargePage,
								 char bChipSelect, uint32_t read_page_delay_us);
        void NAND_AddReadStatus(char * pbyStatus, uint8_t bPrimary);
        //char NAND_EraseBlock(uint32_t dwBlockNumber, uint8_t bChip, uint8_t bPrimary);
        void NAND_EraseBlock(uint32_t dwBlockNumber, uint32_t dwBlockSize, uint32_t dwBlockCount, bool *enabled, char *status);
//        char NAND_ProgramPage(uint32_t dwPage, char * pbyData, uint8_t bChip, uint8_t bPrimary);
//        char NAND_ProgramBlock(uint32_t dwBlock, char * pbyData, uint8_t bChip, uint8_t bPrimary);
        void NAND_ProgramBlock(uint32_t dwBlock, char ** pbSrc, uint32_t dwBlockCount, uint32_t dwBlockSize, uint16_t wPageSize, bool bLargePage, char *status);

        //void NAND_ReadBlock(uint32_t dwBlock, char ** pbDest);
        void NAND_ReadBlock(uint32_t dwBlock, char ** pbDest, uint32_t dwBlockCount, uint32_t dwBlockSize, uint16_t wPageSize, bool bLargePage);


        //void NAND_ReadDualBlock(uint32_t dwBlock1, uint32_t dwBlock2, char * pbDest1, char * pbDest2);

	

        uint32_t Debug_TestShorts();

        void Nopulate(uint32_t nops);
        void Delay_ns(uint32_t delay);
        void Delay_us(uint32_t delay);
        void Delay_ms(uint32_t delay);
	

        int GetTxSpeed();
        int GetRxSpeed();


        void PadTxBuffer( uint32_t count );
        void PadRxBuffer( uint32_t count );
        void EndOfFrame(uint32_t count);

        void SF_Program_Page(uint32_t address, uint32_t len, uint8_t *src);
        void SF_Read(uint32_t address, uint32_t len, uint8_t *dest);
        void SF_ReadID(uint8_t *dest);

void Glitch_Set(uint16_t start, uint16_t end);
// Device vendor and product id.


#define NAND_MFR_TOSHIBA        0x98
#define NAND_MFR_SAMSUNG        0xec
#define NAND_MFR_FUJITSU        0x04
#define NAND_MFR_NATIONAL       0x8f
#define NAND_MFR_RENESAS        0x07
#define NAND_MFR_STMICRO        0x20
#define NAND_MFR_HYNIX          0xad
#define NAND_MFR_MICRON         0x2c
#define NAND_MFR_AMD            0x01

struct nand_manufacturers {
        int id;
        const char * name;
};


//#define BuildVersion 0x0102 // first public version using version
//#define BuildVersion 0x0103 // added timeout to WaitRdy
#define BuildVersion 0x0104 // using a newer approach to the FSM. great improvements in timing analysis.


//#define SetOutputs(value)	temp = (1<<28) | value 
//#define SetAddress(address)	temp = (3<<28) | address

#endif /* __FLASHER_H */

