// Flasher.cpp : Defines the class behaviors for the application.
//
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "Flasher.h"
//#include "Arguments.h"


#define MY_VID 0x1988
#define MY_PID 0x0001

// Device configuration and interface id.
#define MY_CONFIG 1
#define MY_INTF 0

// Device endpoint(s)

#define EP_OUT 0x01
#define EP_IN 0x82
#define EP_CONTROL 0x03

#define TX_BUF_SIZE (1024 * 1024) /* 1 MiB Tx Buffer */
#define TX_THRESHOLD (15 * 1024) /* 16 KiB Tx Threshold */


#define	NAND0_RDY		(0 << 0)
#define NAND0_CLE		(1 << 0)
#define NAND0_ALE		(1 << 1)
#define NAND0_WP		(1 << 2)
#define	NAND0_CE		(1 << 3)
#define SB_TRI_N		(1 << 4)

#define	NAND1_RDY		(1 << 13)
#define NAND1_CLE		(1 << 12)
#define NAND1_ALE		(1 << 11)
#define NAND1_WP		(1 << 10)
#define	NAND1_CE		(1 << 9)

//
//#define	NAND_RDY		(1 << 0)
//#define NAND_CLE		(1 << 1)
//#define NAND_ALE		(1 << 2)
//#define NAND_WP		(1 << 3)
//#define	NAND_CE		(1 << 4)



struct ReadLocation
{
    char * pAddress;
    uint32_t dwSize;
    struct ReadLocation *next;
};

struct NAND_Settings {
    uint16_t wPageSize;
    uint32_t dwBlockSize;
    uint32_t dwBlockCount;
    uint8_t bLargePage;
    uint8_t byAddressCycles;
    uint8_t bChipSelect;
    uint32_t read_page_delay_us;
} g_NAND0_Settings, g_NAND1_Settings;

struct CommonArgs g_Common_Settings;
struct NorArgs g_NOR_Settings;

ReadLocation *g_rlStart = 0;


unsigned char * g_TxBuffer;
uint8_t g_bConfig;
uint32_t g_TxSize = 0;
uint32_t g_RxSize = 0;
uint32_t g_RxOffset = 0;
uint16_t g_dwOutputs = 0;
uint16_t NAND_RDY;
uint16_t NAND_CLE;
uint16_t NAND_ALE;
uint16_t NAND_WP;
uint16_t NAND_CE;

libusb_context *g_libusb_context;
libusb_device_handle *g_libusb_device_handle;
struct libusb_device_descriptor g_libusb_device_descriptor;

#ifdef DEBUG
pfnDebugOut g_pfnDebugOut = NULL;
#endif /* DEBUG */
	   
bool bCancel = false;
	   
void CancelOperations()
{
    bCancel = true;
}

void DebugSetup(pfnDebugOut out)
{
#ifdef DEBUG
    g_pfnDebugOut = out;
#endif
}

void DebugWrite(const char* format, ...)
{
#ifdef DEBUG
    static char buf[(1024 * 8)];
    va_list argp;

    if (g_pfnDebugOut == NULL)
        return;

    va_start(argp, format);
    vsnprintf(buf, sizeof(buf), format, argp);
    va_end(argp);

    g_pfnDebugOut(buf);
#endif /* DEBUG */
}

uint16_t InitDevice() {
    libusb_reset_device(g_libusb_device_handle);
    if (libusb_kernel_driver_active(g_libusb_device_handle, MY_INTF) == 1)
	    libusb_detach_kernel_driver(g_libusb_device_handle, MY_INTF);
    libusb_set_configuration(g_libusb_device_handle, MY_CONFIG);
    libusb_claim_interface(g_libusb_device_handle, MY_INTF);

    
}
uint8_t CreateDevice()
{

    struct libusb_device *device; 
    g_libusb_device_handle = NULL;
    g_libusb_context = NULL;
    libusb_init(&g_libusb_context);
    libusb_set_debug(g_libusb_context, 2);
    g_libusb_device_handle = libusb_open_device_with_vid_pid(g_libusb_context, MY_VID, MY_PID);
    if (!g_libusb_device_handle)
	    return 0;
    InitDevice();
    device = libusb_get_device(g_libusb_device_handle);
    if(!device) device = libusb_get_device(g_libusb_device_handle);
    if(device) {
    	libusb_get_device_descriptor(device, &g_libusb_device_descriptor);
    	printf("descriptor: %04X\n", g_libusb_device_descriptor.bcdDevice);
    }
    g_TxSize = 0;
    g_RxSize = 0;

    g_TxBuffer = (unsigned char*)malloc(TX_BUF_SIZE);

    bCancel = false;

    return 1;
}

void RemoveDevice()
{
    SetConfig(5, 0, 1, 0, 0); // Put ProgSkeet in tristate mode... important for fast testing
    TxStart();

    free(g_TxBuffer);

    libusb_close(g_libusb_device_handle);
    libusb_exit(g_libusb_context);
}

void RxStart()
{
    unsigned char* buf;
    uint32_t rxoffset;
    int read, res;
    ReadLocation* rloc;
    //printf("RxStart with TxSize = %d and RxSize = %d\n", g_TxSize, g_RxSize); fflush(stdout);

    if (g_TxSize)
        TxStart();

    if (!g_RxSize)
        return;

    bCancel = false;

    buf = (unsigned char*)malloc(g_RxSize);
    for (rxoffset = 0, bCancel = false; rxoffset < g_RxSize && !bCancel;) {
	//printf("RX: Call to libusb_bulk_transfer offset %d rxsize %d\n", rxoffset, g_RxSize); fflush(stdout);
        res = libusb_bulk_transfer(g_libusb_device_handle, EP_IN,
                             (unsigned char*)buf + rxoffset, g_RxSize - rxoffset,
                             &read, 1000);

	//printf("RxStart(res=%d) read = %d\n", res, read); fflush(stdout);
        if (res < 0) {
            break; /* TODO: Pass error */
	}

        rxoffset += read;
    }
    // useful to debug data in
    printf("RX:(%08d) ", g_RxSize); for(int i = 0 ; i< g_RxSize ; printf("%02X", buf[i++])); printf("\n");

    rloc = g_rlStart;
    rxoffset = 0;
    while (rloc) {
        memcpy(rloc->pAddress, buf + rxoffset, rloc->dwSize);
        rxoffset += rloc->dwSize;
        g_rlStart = rloc->next;
        free(rloc);
        rloc = g_rlStart;
    }

    free(buf);

    g_RxSize = 0;
    g_RxOffset = 0;
}

void TxStart()
{
    uint32_t txoffset;
    int written, res;

    if(g_TxSize < 1)
        return;

    //printf("TxStart(g_TxSize=%u)\n", g_TxSize); fflush(stdout);

    // usefull to debug data out 
    printf("TX:(%08d) ", g_TxSize); for(int i = 0 ; i< g_TxSize ; printf("%02X", (unsigned char)g_TxBuffer[i++])); printf("\n");
    for (txoffset = 0, bCancel = false; txoffset < g_TxSize && !bCancel;) {
	//printf("TX: Call to libusb_bulk_transfer offset %d rxsize %d\n", txoffset, g_RxSize); fflush(stdout);
        res = libusb_bulk_transfer(g_libusb_device_handle, EP_OUT,
                             (unsigned char*)g_TxBuffer + txoffset, g_TxSize - txoffset,
                             &written, 1000);

	//printf("TxStart(res=%d)\n", res); fflush(stdout);
        if (res < 0) {
	    //printf("TxStart(res=%d)\n", res); fflush(stdout);
            break; /* TODO: Pass error */
	}

        txoffset += written;
    }

    g_TxSize = 0;

    //printf("TxStart finished\n"); fflush(stdout);
}

void TxStartThreshold()
{
    if(g_TxSize < TX_THRESHOLD)
        return;

    RxStart();
}

void TxEnqueueByte(char byte)
{
//    if (g_TxSize < g_Tx)
    g_TxBuffer[g_TxSize++] = (char) 0xFA;
}

void TxEnqueueBuffer(const char* buf, const uint32_t len)
{
}

void Write(uint32_t dwCount, const char * pSrc)
{/*
	if(g_bConfig & 0x10)
		dwCount >>= 1;*/
	while(dwCount > 0xFFFF)
	{
		g_TxBuffer[g_TxSize++] = 0x03;
		g_TxBuffer[g_TxSize++] = (char) 0xFF;
		g_TxBuffer[g_TxSize++] = (char) 0xFF;
		memcpy(g_TxBuffer+g_TxSize, pSrc, (g_bConfig & 0x10)?0x1FFFE:0xFFFF);
		pSrc += (g_bConfig & 0x10)?0x1FFFE:0xFFFF;
		g_TxSize += (g_bConfig & 0x10)?0x1FFFE:0xFFFF;
		dwCount -= 0xFFFF;
	}
	g_TxBuffer[g_TxSize++] = 0x03;
	g_TxBuffer[g_TxSize++] = (char) (dwCount >> 0);
	g_TxBuffer[g_TxSize++] = (char) (dwCount >> 8);
	memcpy(g_TxBuffer+g_TxSize, pSrc, (g_bConfig & 0x10)?(dwCount<<1):dwCount);
	g_TxSize += (g_bConfig & 0x10)?(dwCount<<1):dwCount;
}

void WriteToAddress(uint32_t dwAddress, uint16_t wData)
{
	//printf("WriteToAddress(%08x, %04x)\n", dwAddress, wData); fflush(stdout);
        SetAddress(dwAddress, 0);
        //Delay_us(1);
	g_TxBuffer[g_TxSize++] = 0x03;
	g_TxBuffer[g_TxSize++] = 0x01;
	g_TxBuffer[g_TxSize++] = 0x00;
	g_TxBuffer[g_TxSize++] = (wData >> 0) & 0xFF;
	if(g_bConfig & 0x10)
		g_TxBuffer[g_TxSize++] = (wData >> 8) & 0xFF;
}

void ReadFromAddress(uint32_t dwAddress, uint16_t *pwData)
{
	SetAddress(dwAddress, 0);
	Read(1, (char *) pwData);
}

void SetData(uint16_t wData)
{
    g_TxBuffer[g_TxSize++] = 0x03;
    g_TxBuffer[g_TxSize++] = 0x01;
    g_TxBuffer[g_TxSize++] = 0x00;
    g_TxBuffer[g_TxSize++] = (wData >> 0) & 0xFF;
    if(g_bConfig & 0x10)
            g_TxBuffer[g_TxSize++] = (wData >> 8) & 0xFF;
}

void Stb(uint8_t bValue, uint8_t bMirror)
{
	g_TxBuffer[g_TxSize++] = 0x03;
	g_TxBuffer[g_TxSize++] = 0x01;
	g_TxBuffer[g_TxSize++] = 0x00;
	g_TxBuffer[g_TxSize++] = bValue;
        if(g_bConfig & 0x10) {
		if(bMirror)
			g_TxBuffer[g_TxSize++] = bValue;
		else
			g_TxBuffer[g_TxSize++] = 0x00;
        }
}

void Sth(uint16_t wValue, uint8_t bMirror)
{
	g_TxBuffer[g_TxSize++] = 0x03;
        g_TxBuffer[g_TxSize++] = 0x02;
	g_TxBuffer[g_TxSize++] = 0x00;

	g_TxBuffer[g_TxSize++] = (char) (wValue & 0xFF);
        if(g_bConfig & 0x10) {
		if(bMirror)
			g_TxBuffer[g_TxSize] = g_TxBuffer[g_TxSize-1];
		else
			g_TxBuffer[g_TxSize] = 0x00;
		g_TxSize++;
        }

	g_TxBuffer[g_TxSize++] = (char) ((wValue >> 8) & 0xFF);
        if(g_bConfig & 0x10) {
		if(bMirror)
			g_TxBuffer[g_TxSize] = g_TxBuffer[g_TxSize-1];
		else
			g_TxBuffer[g_TxSize] = 0x00;
		g_TxSize++;
        }

}

void Stw(uint32_t dwValue, uint8_t bMirror)
{
	g_TxBuffer[g_TxSize++] = 0x03;
	g_TxBuffer[g_TxSize++] = 0x04;
	g_TxBuffer[g_TxSize++] = 0x00;

	g_TxBuffer[g_TxSize++] = (char) (dwValue & 0xFF);
        if(g_bConfig & 0x10) {
		if(bMirror)
			g_TxBuffer[g_TxSize] = g_TxBuffer[g_TxSize-1];
		else
			g_TxBuffer[g_TxSize] = 0x00;
		g_TxSize++;
        }

	g_TxBuffer[g_TxSize++] = (char) ((dwValue >> 8) & 0xFF);
        if(g_bConfig & 0x10) {
		if(bMirror)
			g_TxBuffer[g_TxSize] = g_TxBuffer[g_TxSize-1];
		else
			g_TxBuffer[g_TxSize] = 0x00;
		g_TxSize++;
        }

	g_TxBuffer[g_TxSize++] = (char) ((dwValue >> 16) & 0xFF);
        if(g_bConfig & 0x10) {
		if(bMirror)
			g_TxBuffer[g_TxSize] = g_TxBuffer[g_TxSize-1];
		else
			g_TxBuffer[g_TxSize] = 0x00;
		g_TxSize++;
        }
	g_TxBuffer[g_TxSize++] = (char) ((dwValue >> 24) & 0xFF);
        if(g_bConfig & 0x10) {
		if(bMirror)
			g_TxBuffer[g_TxSize] = g_TxBuffer[g_TxSize-1];
		else
			g_TxBuffer[g_TxSize] = 0x00;
		g_TxSize++;
        }
}

void Read(uint32_t dwCount, char * pbyDestination)
{
	//printf("Read(%d, %08x)\n", dwCount, pbyDestination); fflush(stdout);
	if(!g_RxOffset)
		g_RxOffset = g_TxSize;

	uint32_t dwTemp = dwCount;
	while(dwTemp > 0xFFFF)
	{
		g_TxBuffer[g_TxSize++] = 0x04;
		g_TxBuffer[g_TxSize++] = 0xFF;
		g_TxBuffer[g_TxSize++] = 0xFF;
		dwTemp -= 0xFFFF;
	}
	

	//if(dwCount > 0xFFFF)
	//{
	//	Read(0xFFFF, pbyDestination);
	//	if(g_bConfig & 0x10)
	//		Read(dwCount-0xFFFF, pbyDestination+0x1FFFE);
	//	else
	//		Read(dwCount-0xFFFF, pbyDestination+0xFFFF);
	//}

	g_TxBuffer[g_TxSize++] = 0x04;
	g_TxBuffer[g_TxSize++] = (char) (dwTemp >> 0);
	g_TxBuffer[g_TxSize++] = (char) (dwTemp >> 8);
	//else
	//{
		ReadLocation *rlNew = (ReadLocation *) malloc(sizeof(ReadLocation));
		ReadLocation *rltemp = g_rlStart;
		rlNew->pAddress = pbyDestination;
		rlNew->next = NULL;
		if(g_bConfig & 0x10)
			rlNew->dwSize = dwCount * 2;
		else
			rlNew->dwSize = dwCount;
		
		g_RxSize += rlNew->dwSize;

		if(rltemp)
		{
			while(rltemp->next)
				rltemp = rltemp->next;
			rltemp->next = rlNew;
		}
		else
			g_rlStart = rlNew;
	//}
	//printf("Read finish\n"); fflush(stdout);
}

void SetDirections(uint16_t wDirections)
{
        g_TxBuffer[g_TxSize++] = 0x07;
        g_TxBuffer[g_TxSize++] = (wDirections >> 0) & 0xFF;
        g_TxBuffer[g_TxSize++] = (wDirections >> 8) & 0xFF;
}

void SetTrigger(uint16_t wMask, uint16_t wValue)
{
        if(!wMask)
                return;
        g_TxBuffer[g_TxSize++] = 0x08;
	g_TxBuffer[g_TxSize++] = (wValue >> 0) & 0xFF;
	g_TxBuffer[g_TxSize++] = (wValue >> 8) & 0xFF;
	g_TxBuffer[g_TxSize++] = (wMask >> 0) & 0xFF;
	g_TxBuffer[g_TxSize++] = (wMask >> 8) & 0xFF;
}

void Nop(uint8_t byAmount)
{
	g_TxBuffer[g_TxSize++] = 0x09;
	g_TxBuffer[g_TxSize++] = (unsigned char)byAmount & 0xff;
}

void SetAddress(uint32_t dwAddress, uint8_t bAutoIncrement)
{
	//uint32_t temp = dwAddres & 0x7FFFFF
	//printf("SetAddress(%08x, %d)\n", dwAddress, bAutoIncrement); fflush(stdout);
	if(bAutoIncrement)
		dwAddress |= (1 << 23);
	g_TxBuffer[g_TxSize++] = 0x02;
	g_TxBuffer[g_TxSize++] = (char) (dwAddress >> 0);
	g_TxBuffer[g_TxSize++] = (char) (dwAddress >> 8);
	g_TxBuffer[g_TxSize++] = (char) (dwAddress >> 16);
}

void SetOutputs(uint16_t wOutputs)
{
    if(g_dwOutputs != wOutputs)
    {
        g_TxBuffer[g_TxSize++] = 0x06;
        g_TxBuffer[g_TxSize++] = (wOutputs >> 0) & 0xFF;
        g_TxBuffer[g_TxSize++] = (wOutputs >> 8) & 0xFF;
        g_dwOutputs = wOutputs;
    }
}

void GetInputs(uint16_t *pwDestination)
{
	g_TxBuffer[g_TxSize++] = 0x01;

	ReadLocation *rlNew = (ReadLocation *) malloc(sizeof(ReadLocation));
	ReadLocation *rltemp = g_rlStart;
	rlNew->pAddress = (char *) pwDestination;
	rlNew->next = NULL;
	rlNew->dwSize = 2;
	
	g_RxSize += rlNew->dwSize;

	if(rltemp)
	{
		while(rltemp->next)
			rltemp = rltemp->next;
		rltemp->next = rlNew;
	}
	else
		g_rlStart = rlNew;

}

void SetConfig(uint8_t byDelay, uint8_t bDouble, uint8_t bTristate, uint8_t bWaitReady, uint8_t bByteSwap)
{
	uint8_t temp = byDelay & 0xF;
	if(bDouble)
		temp |= (1 << 4);
	if(bTristate)
		temp |= (1 << 5);
	if(bWaitReady)
		temp |= (1 << 6);
	if(bByteSwap)
	{
		temp |= (1 << 7);
		NAND_RDY = NAND1_RDY;
		NAND_CLE = NAND1_CLE;
		NAND_ALE = NAND1_ALE;
		NAND_WP = NAND1_WP;
		NAND_CE = NAND1_CE;
	}
	else
	{
		NAND_RDY = NAND0_RDY;
		NAND_CLE = NAND0_CLE;
		NAND_ALE = NAND0_ALE;
		NAND_WP = NAND0_WP;
		NAND_CE = NAND0_CE;
	}

        if(temp != g_bConfig)
        {
            g_bConfig = temp;
            g_TxBuffer[g_TxSize++] = 0x05;
            g_TxBuffer[g_TxSize++] = temp;
        }
}

void NAND_ReadID(char * pbyDest, uint8_t byCount, uint8_t bChip)
{
	SetConfig(10, 0, 1, 1, bChip);
	SetDirections(NAND_CE | NAND_WP | NAND_ALE | NAND_CLE);
	SetOutputs(NAND_CE | NAND_WP);
	SetConfig(10, 0, 0, 0, bChip);
	SetOutputs(NAND_WP);
	SetOutputs(NAND_CLE | NAND_WP);
	Stb(0x90, 0);
	SetOutputs(NAND_ALE | NAND_WP);
	Stb(0, 0);
	SetOutputs(NAND_WP);
	Read(byCount, pbyDest);
	SetConfig(10, 0, 1, 0, bChip);
	RxStart();
}




void NAND_AddReadStatus(char * pbyStatus)
{
	SetOutputs(NAND_CLE | NAND_WP);
	Stb(0x70, 0);
	Read(1, pbyStatus);
}

void NAND_ReadDualBlock(uint32_t dwBlock1, uint32_t dwBlock2, char * pbDest1, char * pbDest2)
{
    NAND_Settings *m_NAND_Settings;
    m_NAND_Settings = &g_NAND0_Settings;

    SetConfig(5, 0, 0, 0, 0);
    SetDirections(NAND0_CLE | NAND0_ALE | NAND0_WP | NAND0_CE |
                  NAND1_CLE | NAND1_ALE | NAND1_WP | NAND1_CE |
                  SB_TRI_N);

    //uint32_t dwPage = dwBlock * m_NAND_Settings->dwBlockSize;

    uint32_t dwPageCount1, dwPageCount2;
    dwPageCount1 = g_NAND0_Settings.dwBlockSize;
    dwPageCount2 = g_NAND1_Settings.dwBlockSize;
    uint32_t dwPage1, dwPage2;
    dwPage1 = dwPageCount1 * dwBlock1;
    dwPage2 = dwPageCount2 * dwBlock2;


    while(dwPageCount1 || dwPageCount2)
    {
        if(dwPageCount1)
        {
            SetConfig(5, 0, 0, 0, 0);
            dwPageCount1--;
            SetOutputs(NAND0_WP | NAND0_CLE |
                       NAND1_CE | NAND1_WP);
            Stb(0, 0);
            SetOutputs(NAND0_WP | NAND0_ALE |
                       NAND1_CE | NAND1_WP);
            if(g_NAND0_Settings.bLargePage)
            {
                    Stb((uint8_t) 0x00, 0);
                    Stb((uint8_t) 0x00, 0);
                    Stb((uint8_t) ((dwPage1 >> 0) & 0xFF), 0);
                    Stb((uint8_t) ((dwPage1 >> 8) & 0xFF), 0);
                    if((g_NAND0_Settings.dwBlockCount * g_NAND0_Settings.dwBlockSize) > 0xFFFF)
                            Stb((uint8_t) ((dwPage1 >> 16) & 0xFF), 0);
                    if((g_NAND0_Settings.dwBlockCount * g_NAND0_Settings.dwBlockSize) > 0xFFFFFF)
                            Stb((char) ((dwPage1 >> 24) & 0xFF), 0);
                    SetOutputs(NAND0_WP | NAND0_CLE |
                               NAND1_CE | NAND1_WP);
                    Stb(0x30, 0);
            }
            else
            {
                    Stb((uint8_t) 0x00, 0);
                    Stb((uint8_t) ((dwPage1 >> 0) & 0xFF), 0);
                    Stb((uint8_t) ((dwPage1 >> 8) & 0xFF), 0);
            }

            dwPage1++;
        }

        if(dwPageCount2)
        {
            SetConfig(5, 0, 0, 0, 1);
            dwPageCount2--;
            SetOutputs(NAND1_WP | NAND1_CLE |
                       NAND0_CE | NAND0_WP);
            Stb(0, 0);
            SetOutputs(NAND1_WP | NAND1_ALE |
                       NAND0_CE | NAND0_WP);
            if(g_NAND1_Settings.bLargePage)
            {
                    Stb((uint8_t) 0x00, 0);
                    Stb((uint8_t) 0x00, 0);
                    Stb((uint8_t) ((dwPage1 >> 0) & 0xFF), 0);
                    Stb((uint8_t) ((dwPage1 >> 8) & 0xFF), 0);
                    if((g_NAND1_Settings.dwBlockCount * g_NAND1_Settings.dwBlockSize) > 0xFFFF)
                            Stb((uint8_t) ((dwPage2 >> 16) & 0xFF), 0);
                    if((g_NAND1_Settings.dwBlockCount * g_NAND1_Settings.dwBlockSize) > 0xFFFFFF)
                            Stb((char) ((dwPage2 >> 24) & 0xFF), 0);
                    SetOutputs(NAND1_WP | NAND1_CLE |
                               NAND0_CE | NAND0_WP);
                    Stb(0x30, 0);
            }
            else
            {
                    Stb((uint8_t) 0x00, 0);
                    Stb((uint8_t) ((dwPage1 >> 0) & 0xFF), 0);
                    Stb((uint8_t) ((dwPage1 >> 8) & 0xFF), 0);
            }
            dwPage2++;
        }

        if(dwPageCount1--)
        {
            SetOutputs(NAND0_WP |
                       NAND1_CE | NAND1_WP);
	    Nop(255);
            SetConfig(5, 0, 0, 0, 0);
	    Delay_us(g_NAND0_Settings.read_page_delay_us);
            Read(g_NAND0_Settings.wPageSize, pbDest1);
        }
        if(dwPageCount2--)
        {
            SetOutputs(NAND1_WP |
                       NAND0_CE | NAND0_WP);
	    Nop(255);
            SetConfig(5, 0, 0, 0, 1);
	    Delay_us(g_NAND1_Settings.read_page_delay_us);
            Read(g_NAND1_Settings.wPageSize, pbDest2);
        }
	RxStart();

    }
    SetOutputs(NAND0_CE | NAND0_WP |
               NAND1_CE | NAND1_WP);
    //SetConfig(5, 0, 1, 0, 0); // Put ProgSkeet in tristate mode
    TxStart();
    RxStart();
}

void NAND_ReadBlock(uint32_t dwBlock, char * pbDest, char bChip)
{
	NAND_Settings *m_NAND_Settings;
	//printf(">>>>>>>>>>> in NAND_ReadBlock(%d, pbDest, %d\n", dwBlock, bChip);
	if(bChip)
		m_NAND_Settings = &g_NAND1_Settings;
	else
		m_NAND_Settings = &g_NAND0_Settings;

	//printf("Settings: dwBlockSize = %d LargePage = %d BlockCount = %d PageSize = %d\n", m_NAND_Settings->dwBlockSize, m_NAND_Settings->bLargePage, m_NAND_Settings->dwBlockCount, m_NAND_Settings->wPageSize);
	SetConfig(5, 0, 0, 0, bChip);
	SetDirections(NAND_CLE | NAND_ALE | NAND_WP | NAND_CE | SB_TRI_N);
	SetOutputs(NAND_CE | NAND_WP);
	SetOutputs(NAND_WP);

	uint32_t dwPage = dwBlock * m_NAND_Settings->dwBlockSize;
	//printf("dwPage = %d\n", dwPage);

	for(uint32_t i=0; i<m_NAND_Settings->dwBlockSize; i++)
	{
		//printf("page %d/%d\n", i, m_NAND_Settings->dwBlockSize);
		SetOutputs(NAND_WP | NAND_CLE);
		Stb(0, 0);
		SetOutputs(NAND_WP | NAND_ALE);
		if(m_NAND_Settings->bLargePage)
		{
			Stb((uint8_t) 0x00, 0); 
			Stb((uint8_t) 0x00, 0);
			Stb((uint8_t) ((dwPage >> 0) & 0xFF), 0);
			Stb((uint8_t) ((dwPage >> 8) & 0xFF), 0);
			if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFF)
				Stb((uint8_t) ((dwPage >> 16) & 0xFF), 0);
			if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFFFF)
				Stb((char) ((dwPage >> 24) & 0xFF), 0);
			SetOutputs(NAND_WP | NAND_CLE);
			Stb(0x30, 0);
		}
		else
		{
			Stb((uint8_t) 0x00, 0);
			Stb((uint8_t) ((dwPage >> 0) & 0xFF), 0);
			Stb((uint8_t) ((dwPage >> 8) & 0xFF), 0);
		}

		
		SetOutputs(NAND_WP);
		Nop(255);

		SetConfig(10, 0, 0, !bChip, bChip);
		if(bChip)
			SetTrigger(NAND_RDY, NAND_RDY);
		//printf("delay %d\n", m_NAND_Settings->read_page_delay_us);
		Delay_us(m_NAND_Settings->read_page_delay_us);
		
		
		//SetConfig(10, 0, 0, 1, bChip);

		Read(m_NAND_Settings->wPageSize, pbDest);
		//SetConfig(10, 0, 0, 0, bChip);
		dwPage++;
		pbDest += m_NAND_Settings->wPageSize;
		//if(g_bConfig & 0x10)
		//	pbDest += m_NAND_Settings->wPageSize;
		
	//	SetOutputs(NAND_CE | NAND_WP);
		//RxConditional();
		RxStart();
		//Nop(5);
	}
	SetOutputs(NAND_CE | NAND_WP);
	//SetConfig(10, 0, 1, 0, bChip);
	TxStart();
	RxStart();
}

//uint16_t NAND_EraseBlock(uint32_t dwBlockNumber1, uint32_t dwBlockNumber2)
//{
//		NAND_Settings *m_NAND_Settings;
//	if(bChip)
//		m_NAND_Settings = &g_NAND1_Settings;
//	else
//		m_NAND_Settings = &g_NAND0_Settings;
//
//	SetConfig(10, 0, 0, 0, bChip);
//	SetDirections(NAND_CLE | NAND_ALE | NAND_WP | NAND_CE | SB_TRI_N);
//
//	SetOutputs(NAND_CE | NAND_WP);
//	SetOutputs(NAND_WP);
//	SetOutputs(NAND_CLE | NAND_WP);
//	Stb(0x60, 0);
//	SetOutputs(NAND_ALE | NAND_WP);
//	dwBlockNumber *= m_NAND_Settings->dwBlockSize;
//	Stb((dwBlockNumber>>0) & 0xFF, 0);
//	Stb((dwBlockNumber>>8) & 0xFF, 0);
//	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFF)
//		Stb((dwBlockNumber>>16) & 0xFF, 0);
//	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFFFF)
//		Stb((dwBlockNumber>>24) & 0xFF, 0);
//	SetOutputs(NAND_CLE | NAND_WP);
//	Stb(0xD0, 0);
//	SetOutputs(NAND_WP);
//	Nop(255);
//
//	m_NAND_Settings = &g_NAND1_Settings;
//	SetConfig(10, 0, 0, 0, 1);
//	SetDirections(NAND_CLE | NAND_ALE | NAND_WP | NAND_CE | SB_TRI_N);
//	SetOutputs(NAND_CE | NAND_WP);
//	SetOutputs(NAND_WP);
//	SetOutputs(NAND_CLE | NAND_WP);
//	Stb(0x60, 0);
//	SetOutputs(NAND_ALE | NAND_WP);
//	dwBlockNumber *= m_NAND_Settings->dwBlockSize;
//	Stb((dwBlockNumber>>0) & 0xFF, 0);
//	Stb((dwBlockNumber>>8) & 0xFF, 0);
//	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFF)
//		Stb((dwBlockNumber>>16) & 0xFF, 0);
//	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFFFF)
//		Stb((dwBlockNumber>>24) & 0xFF, 0);
//	SetOutputs(NAND_CLE | NAND_WP);
//	Stb(0xD0, 0);
//	SetOutputs(NAND_WP);
//	Nop(255);
//
//	//if(bChip)
//	//	SetTrigger(NAND_RDY, NAND_RDY);
//	//SetConfig(10, 0, 0, !bChip, bChip);
//	//Delay_ms(3);
//	char status = 0;
//	NAND_AddReadStatus(&status);
//	RxStart();
//
//	return status;
//}

char NAND_EraseBlock(uint32_t dwBlockNumber, char bChip)
{
	NAND_Settings *m_NAND_Settings;
	if(bChip)
		m_NAND_Settings = &g_NAND1_Settings;
	else
		m_NAND_Settings = &g_NAND0_Settings;

	SetConfig(10, 0, 0, 0, bChip);
	SetDirections(NAND_CLE | NAND_ALE | NAND_WP | NAND_CE | SB_TRI_N);

	SetOutputs(NAND_CE | NAND_WP);
	SetOutputs(NAND_WP);
	SetOutputs(NAND_CLE | NAND_WP);
	Stb(0x60, 0);
	SetOutputs(NAND_ALE | NAND_WP);
	dwBlockNumber *= m_NAND_Settings->dwBlockSize;
	Stb((dwBlockNumber>>0) & 0xFF, 0);
	Stb((dwBlockNumber>>8) & 0xFF, 0);
	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFF)
		Stb((dwBlockNumber>>16) & 0xFF, 0);
	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFFFF)
		Stb((dwBlockNumber>>24) & 0xFF, 0);
	SetOutputs(NAND_CLE | NAND_WP);
	Stb(0xD0, 0);
	SetOutputs(NAND_WP);
	Nop(255);
	//if(bChip)
	//	SetTrigger(NAND_RDY, NAND_RDY);
	//SetConfig(10, 0, 0, !bChip, bChip);
	Delay_ms(3);
	char status = 0;
	NAND_AddReadStatus(&status);
	RxStart();

	return status;
}

char NAND_ProgramPage(uint32_t dwPage, char * pbyData, char bChip)
{
	NAND_Settings *m_NAND_Settings;
	if(bChip)
		m_NAND_Settings = &g_NAND1_Settings;
	else
		m_NAND_Settings = &g_NAND0_Settings;

	//SetOutputs(NAND_CE | NAND_WP);
	SetConfig(10, 0, 0, 0, bChip);

	uint16_t wOffset = 0;
	while(pbyData[wOffset] == (char)0xFF)
		wOffset++;

	uint16_t wEnd = m_NAND_Settings->wPageSize-1;
	while(pbyData[wEnd] == (char)0xFF)
		wEnd--;
	SetOutputs(NAND_WP);
	SetOutputs(NAND_CLE | NAND_WP);
	Stb(0x80, 0);
	SetOutputs(NAND_ALE | NAND_WP);
	Stb((wOffset >> 0) & 0xFF, 0);
	if(m_NAND_Settings->bLargePage)
		Stb((wOffset >> 8) & 0xFF, 0);
	Stb((dwPage >> 0) & 0xFF, 0);
	Stb((dwPage >> 8) & 0xFF, 0);
	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFF)
		Stb((dwPage >> 16) & 0xFF, 0);
	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFFFF)
		Stb((dwPage >> 24) & 0xFF, 0);
	SetOutputs(NAND_WP);
	Write(wEnd-wOffset+1, pbyData+wOffset);
	SetOutputs(NAND_WP | NAND_CLE);
	Stb(0x10, 0);
	SetOutputs(NAND_WP);
	Delay_us(800);
	return 0;
}

char NAND_ProgramBlock(uint32_t dwBlock, char * pbyData, char bChip)
{
	NAND_Settings *m_NAND_Settings;
	if(bChip)
		m_NAND_Settings = &g_NAND1_Settings;
	else
		m_NAND_Settings = &g_NAND0_Settings;

	/*char * pbyActualContent = (char *) malloc(m_NAND_Settings->dwBlockSize * m_NAND_Settings->wPageSize);
	NAND_ReadBlock(dwBlock, pbyActualContent, bChip);
	if( memcmp(pbyActualContent, pbyData, m_NAND_Settings->dwBlockSize * m_NAND_Settings->wPageSize) == 0 )
	{
		free(pbyActualContent);
		return 0;
	}*/


//	SetConfig(10, 0, 0, !bChip, bChip);
	SetDirections(NAND_CLE | NAND_ALE | NAND_WP | NAND_CE | SB_TRI_N);
	SetOutputs(NAND_WP | NAND_CE);
	SetOutputs(NAND_WP);

	char * pbyTemp = (char *) malloc(m_NAND_Settings->wPageSize);
	memset(pbyTemp, 0xFF, m_NAND_Settings->wPageSize);

	char * status = (char *) malloc(m_NAND_Settings->dwBlockSize);
	memset(status, 0, m_NAND_Settings->dwBlockSize);

	for(uint32_t i=0; i<m_NAND_Settings->dwBlockSize; i++)
		if(memcmp(pbyData+(i*m_NAND_Settings->wPageSize), pbyTemp, m_NAND_Settings->wPageSize))
		{
			NAND_ProgramPage((dwBlock*m_NAND_Settings->dwBlockSize)+i, pbyData+(i*m_NAND_Settings->wPageSize), bChip);
			Nop(255);
			//if(bChip)
			//	SetTrigger(NAND_RDY, NAND_RDY);
			NAND_AddReadStatus(status+i);
			RxStart();
			
		}

	//RxStart();
	char res = 0;
	for(uint32_t i=0; i<m_NAND_Settings->dwBlockSize; i++)
		res |= *(char *)(status+i);
	free(pbyTemp);
	free(status);
	return res;// & 0x1;
}





void NAND_Configure(uint16_t wPageSize, uint32_t dwBlockSize, uint32_t dwBlockCount, char bLargePage,
								 char bChipSelect, uint32_t read_page_delay_us)
{
    SetPinout(1);
	if(bChipSelect) {
		g_NAND1_Settings.bLargePage = (bLargePage != 0);
		g_NAND1_Settings.byAddressCycles = bLargePage?5:3;
		g_NAND1_Settings.dwBlockSize = dwBlockSize;
		g_NAND1_Settings.dwBlockCount = dwBlockCount;
		g_NAND1_Settings.wPageSize = wPageSize;
		g_NAND1_Settings.read_page_delay_us = read_page_delay_us;
	}
	else {
		g_NAND0_Settings.bLargePage = (bLargePage != 0);
		g_NAND0_Settings.byAddressCycles = bLargePage?5:3;
		g_NAND0_Settings.dwBlockSize = dwBlockSize;
		g_NAND0_Settings.dwBlockCount = dwBlockCount;
		g_NAND0_Settings.wPageSize = wPageSize;
		g_NAND0_Settings.read_page_delay_us = read_page_delay_us;
	}
}

uint32_t Debug_TestShorts()
{
	uint16_t g_result[16];
	uint16_t d_result[16];
        uint32_t result = 0;
	SetConfig(10, 1, 0, 0, 0);

	for(int i=0; i<16; i++) {
		WriteToAddress(0, 1<<i);
		Delay_us(20);
		Read(1, (char *) &d_result[i]);
		SetDirections(0xFFFF);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);
		SetOutputs(0);

		
		Nop(255);
		Nop(255);
		SetOutputs(1<<i);
		SetDirections(1<<i);
		Delay_us(20);
		SetDirections(0);
		GetInputs(&g_result[i]);
	}
	RxStart();

	for(int i=0; i<16; i++) {
		if(d_result[i] != (1<<i))
                    result |= (1 << i);
		if(g_result[i] != (1<<i))
                    result |= (1 << (i+16));
	}
        return result;
}

void Nopulate(uint32_t nops)
{
    while (nops >= 255) {
        Nop(255);
        nops -= 255;
    }

    Nop((unsigned char)nops&0xff);
}

void Delay_ns(uint32_t delay)
{
    static const double ratio = 20.833333333333333333333333333333;
    Nopulate((delay / ratio) + 1); /* Better 1 more than one less */
}

void Delay_us(uint32_t delay)
{
    Nopulate(delay * 48);
}

void Delay_ms(uint32_t delay)
{
    Nopulate(delay * 47940);
}

void NOR_Configure(struct CommonArgs common, struct NorArgs nor)
{
    SetPinout(0);
    g_Common_Settings = common;
    g_NOR_Settings = nor;

    /* Statically configure the timings, needs configuration */
    g_NOR_Settings.timings.tBUSY = 90; /* ns */
    g_NOR_Settings.timings.tRB = 50; /* ns, only needed for Macronix */
    g_NOR_Settings.timings.tWHWH1_BufWord = 64; /* us, max = 2048us for 64 byte buffer */
    g_NOR_Settings.timings.tWHWH1_Word = 64;
}

void NOR_Read(char* buf, uint32_t addr, uint32_t len)
{
    //printf("NOR_Read start\n"); fflush(stdout);
    addr >>= g_Common_Settings.word;
    len >>= g_Common_Settings.word;
    uint16_t out = NOR_DEFAULT_OUT | NOR_CE;
    SetOutputs(out);
    SetAddress(addr, 1);
    SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, g_Common_Settings.byteSwap);
    Delay_us(1);
    out ^= NOR_CE;
    SetOutputs(out);
    Delay_us(1);
//    WriteToAddress(0x00, 0x0F); Nop(255);

    Read(len, buf);
 //   SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, 0);

    RxStart();
}

void NOR_Wait(uint32_t addr, uint32_t lenw)
{
    char *buf = (char*) malloc(2);

    switch (g_NOR_Settings.waitMethod) {
    case NorWaitMethod_RdyTrigger:
        SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, g_Common_Settings.byteSwap);

//        /* Charge cap for trigger */
//        SetDirections(NOR_DEFAULT_DIR | NOR_ALT_RDY);
//        SetOutputs(NOR_DEFAULT_OUT | NOR_ALT_RDY);

//        /* tBUSY */
//        //Delay_ns(g_NOR_Settings.timings.tBUSY);
//        Delay_us(5);

//        /* Trigger wait for NOR RDY (tWHWH1) */
//        SetDirections(NOR_DEFAULT_DIR); Nop(255);
//        SetTrigger(NOR_ALT_RDY, NOR_ALT_RDY);
        break;
    case NorWaitMethod_DataPolling:
        /* tBUSY */
        Delay_ns(g_NOR_Settings.timings.tBUSY);

        SetAddress(addr, 0);
        SetConfig(g_Common_Settings.period, 0, 0, 1, 0);

        /* Poll data */

        do {
            Read(2, buf);
            RxStart();
        } while(memcmp(buf, buf+1, 1));

        SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, 0);
        break;
    case NorWaitMethod_Static:
        /* tWHWH1 * word count */
        if (g_NOR_Settings.writeMethod == NorWriteMethod_BufferedWrite) {
            Delay_us(g_NOR_Settings.timings.tWHWH1_BufWord * lenw);
        } else {
            Delay_us(g_NOR_Settings.timings.tWHWH1_Word * lenw);
        }
        break;
    default:
    case NorWaitMethod_TxStart:
        TxStart();
        break;
    }

//#define WAIT_DEBUG
#ifdef WAIT_DEBUG
    /* Toggle GP5 for 1us for debug */
    SetDirections(NOR_DEFAULT_DIR | NOR_GP(5));
    SetOutputs(NOR_DEFAULT_OUT | NOR_GP(5));
    Nop(48);
    SetDirections(NOR_DEFAULT_DIR);
#endif
    free(buf);
}

void NOR_BufferWrite(uint32_t ebaddr, uint32_t addr, const char* data, uint32_t len)
{
    addr >>= g_Common_Settings.word;
    len >>= g_Common_Settings.word;

    uint32_t saddr = addr & ~(len - 1);

    DebugWrite("NOR_BufferWrite(ebaddr=0x%04x, addr=0x%04x, data=%p, len=%u)", ebaddr, addr, data, len);

    WriteToAddress(0x555 | ebaddr, 0xAA);
    WriteToAddress(0x2AA | ebaddr, 0x55);
    WriteToAddress(saddr, 0x25);
    WriteToAddress(saddr, len - 1);

    SetAddress(addr, 1); Nop(255);

    /* Fill buffer */
    SetConfig(10, g_Common_Settings.word, 0, 0, g_Common_Settings.byteSwap);
    Write(len, data);
    SetConfig(10, g_Common_Settings.word, 0, 0, 0);

    /* Start writing */
    WriteToAddress(addr, 0x29);

    NOR_Wait(addr, len);

    TxStartThreshold();
}

void NOR_Program(uint32_t addr, const char* data, uint32_t len)
{
    uint32_t buf_max, written, addrw, ebaddr;

    uint16_t out = NOR_DEFAULT_OUT | NOR_CE;
    SetOutputs(out);
    out ^= NOR_CE;
    Delay_us(1);
    SetOutputs(out);
    Delay_us(1);


    buf_max = g_NOR_Settings.bufferedMax;
    written = 0;
    ebaddr = addr >> g_Common_Settings.word;

    DebugWrite("NOR_Program(addr=0x%04x, data=%p, len=%u)", addr, data, len);

    SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 0, 0);

    switch (g_NOR_Settings.writeMethod) {
    case NorWriteMethod_BufferedWrite:
        while (len - written >= buf_max) {
            NOR_BufferWrite(ebaddr, addr + written, data + written, buf_max);
            written += buf_max;
        }

        if (len - written > 0)
            NOR_BufferWrite(ebaddr, addr + written, data + written, len - written);

        break;

    case NorWriteMethod_SingleWordProgram:
        while (len - written > 1) {
            if(*(uint16_t*)(data + written) != 0xFFFF) {

                addrw = (addr + written) >> g_Common_Settings.word;

                SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, 0);
                WriteToAddress(0x555 | ebaddr, 0xAA);
                WriteToAddress(0x2AA | ebaddr, 0x55);
                WriteToAddress(0x555 | ebaddr, 0xA0);

                SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, g_Common_Settings.byteSwap);
                WriteToAddress(addrw, *(uint16_t*)(data + written));

                //Delay_us(10);

                TxStartThreshold();
            }
            written += 2;
        }
        TxStart();
        NOR_Wait(addrw, 1);


        break;

    case NorWriteMethod_DoubleWordProgram:
        while (len - written > 3) {
            addrw = (addr + written) >> g_Common_Settings.word;

            SetConfig(10, g_Common_Settings.word, 0, 0, 0);
            WriteToAddress(0x555 | ebaddr, 0x50);

            SetConfig(10, g_Common_Settings.word, 0, 0, g_Common_Settings.byteSwap);
            WriteToAddress(addrw, *(uint16_t*)(data + written));
            WriteToAddress(addrw + (2 >> g_Common_Settings.word), *(uint16_t*)(data + written + 2));
            Delay_us(200);

            NOR_Wait(addrw, 2);

            TxStartThreshold();

            written += 4;
        }
        break;
    default:
        break;
    }
    out |= NOR_CE;
    SetOutputs(out);
    SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, g_Common_Settings.byteSwap);

    TxStart();

    DebugWrite("NOR_Program finished");
}

void NOR_Erase(uint32_t addr)
{
    uint16_t out = NOR_DEFAULT_OUT | NOR_CE;
    SetOutputs(out);
    Nop(10);
    out ^= NOR_CE;
    SetOutputs(out);
    Nop(10);

    addr >>= g_Common_Settings.word;
    SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, 0);
    WriteToAddress(0x555, 0xAA);
    WriteToAddress(0x2AA, 0x55);
    WriteToAddress(0x555, 0x80);
    WriteToAddress(0x555, 0xAA);
    WriteToAddress(0x2AA, 0x55);
    WriteToAddress(addr, 0x30);
//    Delay_us(50);
//    out |= NOR_CE;
//    SetOutputs(out);

    NOR_Wait(addr, 2);
    TxStart();
//    SetConfig(10, 0, 0, 1, 0);

    char buf[2];
    buf[0] = 0xFF;
    buf[1] = 0x00;
    do {
        Read(2, buf);
        RxStart();
    } while(buf[0] != buf[1]);

//    SetConfig(10, g_Common_Settings.word, 0, 1, 0);
}

void NOR_PPBErase()
{
    /* FIXME ? */
    WriteToAddress(0xFF, 0x80);
    WriteToAddress(0, 30);
    TxStart();
}

void NOR_Reset()
{
    SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 0, 0);

    SetDirections(NOR_DEFAULT_DIR);

    uint16_t out = NOR_DEFAULT_OUT;

    out |= NOR_CE; /* CE -> HIGH */
    SetOutputs(out); Nop(255);
    out &= ~NOR_RST; /* RST -> LOW */
    SetOutputs(out); Delay_ms(2);
    out |= NOR_RST; /* RST -> HIGH */
    SetOutputs(out); Delay_ms(2);

    out &= ~NOR_CE; /* CE -> LOW */
    SetOutputs(out); Nop(255);



    SetConfig(10, 1, 0, 1, 0);

    TxStart();
}

#define CFI_DUMP_SIZE 0x100

int NOR_ReadCFI(char* buf, uint32_t len)
{
    if (len < CFI_DUMP_SIZE / 2)
        return 1;

    memset(buf, 0, len);

    SetConfig(10, 0, 0, 1, 0);

    /* Exit CFI query */
    WriteToAddress(0x00, 0x0F);
    /* Start CFI query */
    WriteToAddress(0x55, 0x98);

    Nop(255);

    /* Start at 0x20 (8-bit) / 0x10 (16-bit) */
    SetAddress(0x20 >> g_Common_Settings.word, 1);

    /* Read actual data */
    Read(CFI_DUMP_SIZE >> g_Common_Settings.word, buf);

    /* Exit CFI query */
    WriteToAddress(0x00, 0x0F);

    RxStart();

    return 0;
}

void SetPinout(int Pinout) // 0 = NOR, 1 = NAND
{
    switch(Pinout) {
        case 0: g_TxBuffer[g_TxSize++] = 0x0B; break;
        case 1: g_TxBuffer[g_TxSize++] = 0x0A; break;
    default: break;
    }


}
