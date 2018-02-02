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
//#define TX_THRESHOLD (16 * 1024) /* 16 KiB Tx Threshold */


uint8_t CRITICAL_LEVEL = 0;

#define PORTA                   (1<<0)
#define PORTB                   (1<<1)
#define PORTC                   (1<<2)
#define PORTD                   (1<<3)
#define PORTE                   (1<<4)
#define PORTF                   (1<<5)
#define PORTG                   (1<<6)


#define PORT_NAND0              PORT_

#define	NAND0_RDY		(0 << 0)    //P2    ok  ok
#define NAND0_CLE		(1 << 0)    //P9    ok  ok
#define NAND0_ALE		(1 << 1)    //P8    ok  ok
#define NAND0_WP		(1 << 2)    //P4    ok  ok
#define	NAND0_CE_A		(1 << 3)    //P5    ok  ok
#define NAND0_CE_B              (1 << 4)    //P3    ok  ok

//#define SB_TRI_N		(1 << 4)

#define	NAND1_RDY		(1 << 13)   //P26   ok  ok
#define NAND1_CLE		(1 << 12)   //P33   ok  ok
#define NAND1_ALE		(1 << 11)   //P32   ok  ok
#define NAND1_WP		(1 << 10)   //P28   ok  ok
#define	NAND1_CE_A		(1 << 9)    //P29   ok  ok
#define NAND1_CE_B              (1 << 8)    //P27   ok  ok

#define SPI_CS                  (1 << 7)

uint32_t g_TimeOut = 0;
uint32_t totalcycles = 0;


//
//#define	NAND_RDY		(1 << 0)
//#define NAND_CLE		(1 << 1)
//#define NAND_ALE		(1 << 2)
//#define NAND_WP		(1 << 3)
//#define	NAND_CE		(1 << 4)

uint32_t TX_THRESHOLD = 16384;

bool isCritical = false;
uint32_t CriticalOffset = 0;
uint32_t criticalbuffer = 0;
bool TxBusy = false;

uint16_t NAND_CE_LOW = 0;
uint16_t NAND_CE_HIGH = NAND0_CE_A | NAND0_CE_B | NAND1_CE_A | NAND1_CE_B | NAND0_WP | NAND1_WP;

struct ReadLocation
{
    char * pAddress;
    uint32_t dwSize;
    struct ReadLocation *next;
};


struct allocation
{
    void *location;
    struct allocation *next;
};
allocation *allocationstart = 0;

struct CmdLocation
{
    uint8_t cmd;
    uint32_t args;
    uint8_t argsize;
    uint8_t *src; uint32_t srcsize;
    uint8_t *dest; uint32_t destsize;
    struct CmdLocation *next;
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



unsigned char * g_TxBuffer = 0;
uint8_t g_bConfig;
uint32_t g_TxSize = 0;
uint32_t g_RxSize = 0;
uint32_t g_RxOffset = 0;
uint16_t NAND_RDY;
uint16_t NAND_CLE;
uint16_t NAND_ALE;
uint16_t NAND_WP;
uint16_t NAND_CE, NAND_CE_A, NAND_CE_B;
uint32_t rxspeed;
uint32_t txspeed;
uint16_t g_wOutputs = 0;
uint32_t g_dwAddress = 0;

uint32_t regs[256];
uint8_t *cycles;

    uint8_t g_Confirmation;


uint16_t g_Directions = 0;
uint16_t g_GPIO = 0;


#ifdef use_libusb_win32
usb_dev_handle *handle;

#else

libusb_context *g_libusb_context;
libusb_device_handle *g_libusb_device_handle;
struct libusb_device_descriptor g_libusb_device_descriptor;
#endif


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


#ifndef use_libusb_win32
uint16_t InitDevice() {
    libusb_reset_device(g_libusb_device_handle);
    if (libusb_kernel_driver_active(g_libusb_device_handle, MY_INTF) == 1)
            libusb_detach_kernel_driver(g_libusb_device_handle, MY_INTF);
    libusb_set_configuration(g_libusb_device_handle, MY_CONFIG);
    libusb_claim_interface(g_libusb_device_handle, MY_INTF);
}
#endif


uint8_t CreateDevice()
{
#ifndef use_libusb_win32
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
#else

    usb_dev_handle *hdev = NULL; /* the device handle */
    void* async_read_context = NULL;
    void* async_write_context = NULL;

    usb_set_debug(255);


    usb_init(); /* initialize the library */

    usb_find_busses(); /* find all busses */
    usb_find_devices(); /* find all connected devices */

    struct usb_bus *bus;
    struct usb_device *dev;

    for (bus = usb_get_busses(); bus; bus = bus->next)
    {
        for (dev = bus->devices; dev; dev = dev->next)
        {
            if (dev->descriptor.idVendor == MY_VID
                    && dev->descriptor.idProduct == MY_PID)
            {
                                //debugmessage = "Device found.\r\n";
                                //Debug_Write(debugmessage);
                hdev = usb_open(dev);
            }
        }
    }

    if(hdev == NULL)
    {
//        debugmessage = "Device not found.\r\n";
//        Debug_Write(debugmessage);
        return 0;
    }
    usb_set_configuration(hdev, MY_CONFIG);
    usb_claim_interface(hdev, 0);
    handle = hdev;
#endif


    //if(!g_TxBuffer)
    //{
        g_TxBuffer = (unsigned char*)malloc(TX_BUF_SIZE);
        cycles = (uint8_t*) malloc(256*4);
        g_TxSize = 0;
        g_RxSize = 0;
        isCritical = false;

        bCancel = false;
    //}
    return 1;
}

uint8_t lastcmd = 0;
uint32_t lastsize = 0;



void* mymalloc(uint32_t size)
{
    allocation *newalloc = (allocation*) malloc(sizeof(allocation));
    allocation *temp = allocationstart;
    newalloc->location = malloc(size);
    if(!allocationstart)
        allocationstart = temp;
    else
    {

        while(temp->next)
            temp = temp->next;
        temp->next = newalloc;
    }
}

void myfree(void *address)
{
    /* search for param address and free the address allocation and then free the structure*/
}

void myfree_all()
{
    allocation *temp = allocationstart;
    while(allocationstart)
    {
        temp = allocationstart;
        allocationstart = allocationstart->next;
        free(temp->location);
        free(temp);
    }
}

void EnqueueCommand(uint8_t cmd, uint32_t args, uint8_t argsize,
                    uint8_t *src, uint32_t srcsize,
                    uint8_t *dest, uint32_t destsize,
                    bool critical)
{



    //if(cmd != 0xFE)
//#define benchmark
#ifdef benchmark
    if(cmd != 0xC4) // query timebase... don't use yet, need to find bug in vhdl
        EnqueueCommand(0xC4, 0, 0, 0, 0, (uint8_t *)cycles+(cmd*4)+0, 2, 0);
#endif
    TxStartThreshold(1+argsize+srcsize);
//    if(!isCritical)
//        if(critical)
//        {
//            isCritical = critical;
//            g_RxOffset = g_TxSize;
//        }

        if(!g_RxOffset)
//            if(destsize>63)
                g_RxOffset = g_TxSize;


    regs[cmd] = args;

    g_TxBuffer[g_TxSize++] = cmd;
    while(argsize--) {
        g_TxBuffer[g_TxSize++] = args&0xFF;
        args>>=8;
    }
    if(srcsize) {
        memcpy(g_TxBuffer+g_TxSize, src, srcsize);
        g_TxSize += srcsize;
    }


    if(destsize) {
        ReadLocation *rlNew = (ReadLocation *) malloc(sizeof(ReadLocation));
        ReadLocation *rltemp = g_rlStart;
        rlNew->pAddress = (char *) dest;
        rlNew->next = 0;
        rlNew->dwSize = destsize;

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

// #define verbose

#ifdef verbose
//    EndOfFrame(1);
    RxStart();
#endif

#ifdef benchmark
    if(cmd != 0xC4)
        EnqueueCommand(0xC4, 0, 0, 0, 0, (uint8_t *)cycles+(cmd*4)+2, 2, 0);
#endif

}

void RemoveDevice()
{
    SetConfig(5, 0, 1, 0, 0); // Put ProgSkeet in tristate mode... important for fast testing
    RxStart();

    free(g_TxBuffer);
    free(cycles);
#ifndef use_libusb_win32
    libusb_close(g_libusb_device_handle);
    libusb_exit(g_libusb_context);
#else
    usb_release_interface(handle, 0);
    usb_close(handle);
#endif

}

int GetRxSpeed()
{
    return rxspeed;
}

int GetTxSpeed()
{
    return txspeed;
}

int RxStart()
{


    bCancel = false;
    unsigned char* buf;
    uint32_t rxoffset;
    int res = 0;

    ReadLocation* rloc;
    criticalbuffer = 0;
    //printf("RxStart with TxSize = %d and RxSize = %d\n", g_TxSize, g_RxSize); fflush(stdout);




//    if(isCritical)
//        PadTxBuffer(2047-(g_TxSize-g_RxOffset));
//    else
//        PadTxBuffer(63-(g_TxSize % 64));



    if (g_TxSize)
        res = TxStart();

    if(res<0)
        return res;

    if (!g_RxSize)
        return 0;


    uint32_t elapsed = GetTickCount();


    buf = (unsigned char*)malloc(g_RxSize);
    for (rxoffset = 0, bCancel = false; rxoffset < g_RxSize /*&& !bCancel*/;) {
	//printf("RX: Call to libusb_bulk_transfer offset %d rxsize %d\n", rxoffset, g_RxSize); fflush(stdout);

#ifndef use_libusb_win32
        res = libusb_bulk_transfer(g_libusb_device_handle, EP_IN,
                             (unsigned char*)buf + rxoffset, g_RxSize - rxoffset,
                             &read, 1000);
#else
        res = usb_bulk_read(handle, EP_IN, (char *) buf+rxoffset, g_RxSize-rxoffset, 0);
        if (res < 0) {
            return res;
            //break; /* TODO: Pass error */
        } else
            rxoffset += res;

#endif
	//printf("RxStart(res=%d) read = %d\n", res, read); fflush(stdout);

    }

    elapsed = GetTickCount()-elapsed;

    if(elapsed)
        rxspeed = g_RxSize/elapsed;


    // useful to debug data in
    //printf("RX:(%08d) ", g_RxSize); for(int i = 0 ; i< g_RxSize ; printf("%02X", buf[i++])); printf("\n");

    rloc = g_rlStart;
    rxoffset = 0;
    while (rloc) {
        if(rloc->pAddress)
            memcpy(rloc->pAddress, buf + rxoffset, rloc->dwSize);
        rxoffset += rloc->dwSize;
        g_rlStart = rloc->next;
        free(rloc);
        rloc = g_rlStart;
    }

    free(buf);

    g_RxSize = 0;
    g_RxOffset = 0;

    isCritical = (/*g_bConfig*/regs[0x05] & 0x40)?true:false;
    return rxoffset;
}

void SetCounter(uint16_t count)
{
    if(regs[0x01] == count)
        return;
    EnqueueCommand(0x01, count, 2, 0, 0, 0, 0, 0);
}

void PadTxBuffer( uint32_t count )
{
    totalcycles += (count<<1);
    while(count--)
        EnqueueCommand(0xFF, 0, 0, 0, 0, 0, 0, 1);
//        g_TxBuffer[g_TxSize++] = 0xFF;
}

void PadRxBuffer( uint32_t count )
{
    SetCounter(count-1);
    EnqueueCommand(0xFE, 0, 0, 0, 0, 0, count, 0);
    regs[0x01] = 0;
}

void EndOfFrame(uint32_t count)
{
    totalcycles += (count<<1);


//    if(!isCritical)
//    {
//        g_RxOffset = g_TxSize;
//        isCritical = true;
//    }

    while(count--)
    {
        g_TxBuffer[g_TxSize++] = 0xFE; //EnqueueCommand(0xFE, 0, 0, 0, 0, 0, 1, 1);
        g_RxSize++;
    }



//    ReadLocation *rlNew = (ReadLocation *) malloc(sizeof(ReadLocation));
//    ReadLocation *rltemp = g_rlStart;
//    rlNew->pAddress = 0;
//    rlNew->next = NULL;
//    rlNew->dwSize = count;

//    g_RxSize += rlNew->dwSize;

//    if(rltemp)
//    {
//            while(rltemp->next)
//                    rltemp = rltemp->next;
//            rltemp->next = rlNew;
//    }
//    else
//            g_rlStart = rlNew;



}

void GetTimeBase(uint16_t *dest)
{
    EnqueueCommand(0xC4, 0, 0, 0, 0, (uint8_t *) dest, 2, 0);
}

void GetVersion(uint16_t *dest)
{
    EnqueueCommand(0, 0, 0, 0, 0, (uint8_t *) dest, 2, 1);
//    totalcycles += 3;

//    TxStartThreshold(2);

//    if(!isCritical)
//    {
//        g_RxOffset = g_TxSize;
//        isCritical = true;
//    }

//    g_TxBuffer[g_TxSize++] = 0;

//    ReadLocation *rlNew = (ReadLocation *) malloc(sizeof(ReadLocation));
//    ReadLocation *rltemp = g_rlStart;
//    rlNew->pAddress = (char *) dest;
//    rlNew->next = NULL;
//    rlNew->dwSize = 2;

//    g_RxSize += rlNew->dwSize;

//    if(rltemp)
//    {
//            while(rltemp->next)
//                    rltemp = rltemp->next;
//            rltemp->next = rlNew;
//    }
//    else
//            g_rlStart = rlNew;
}

int TxStart()
{
    uint32_t txoffset;
    int written, res;


    if(g_TxSize < 1)
        return 0;

    //if(!g_RxSize)
    //{
        TxBusy = true;
        EndOfFrame(1);
    //}


//    if(g_RxSize < 16383)
//        PadRxBuffer(16383-g_RxSize);

//    //if(g_TxSize < 16383)
//    if(g_TxSize < 2047)
//    {
//        PadTxBuffer(16383-g_TxSize);
//        EndOfFrame(1);
//     }

    //printf("TxStart(g_TxSize=%u)\n", g_TxSize); fflush(stdout);

    // usefull to debug data out 
    //printf("TX:(%08d) ", g_TxSize); for(int i = 0 ; i< g_TxSize ; printf("%02X", (unsigned char)g_TxBuffer[i++])); printf("\n");

    uint32_t elapsed = GetTickCount();

    for (txoffset = 0, bCancel = false; txoffset < g_TxSize /* && !bCancel*/;) {
	//printf("TX: Call to libusb_bulk_transfer offset %d rxsize %d\n", txoffset, g_RxSize); fflush(stdout);

#ifndef use_libusb_win32
        res = libusb_bulk_transfer(g_libusb_device_handle, EP_OUT,
                             (unsigned char*)g_TxBuffer + txoffset, g_TxSize - txoffset,
                             &written, 1000);
#else
        res = usb_bulk_write(handle, EP_OUT, (char*) g_TxBuffer+txoffset, g_TxSize-txoffset, 10000);


	//printf("TxStart(res=%d)\n", res); fflush(stdout);
        if (res < 0) {
	    //printf("TxStart(res=%d)\n", res); fflush(stdout);
            return res; /* TODO: Pass error */
	}
        else
            txoffset += res;
#endif
    }

    elapsed = GetTickCount()-elapsed;

    if(elapsed)
        txspeed = g_TxSize/elapsed;

    g_TxSize = 0;
//    if(bCancel)
//        RemoveDevice();
    //printf("TxStart finished\n"); fflush(stdout);
    totalcycles = 0;
    TxBusy = false;
    return txoffset;
}

int TxStartThreshold(uint32_t size)
{
//    if(TxBusy)
//        return 0;

    if(g_TxSize+size > 2047)
//        if((criticalbuffer > 2047) || ((g_TxSize+size) > 4191))
                RxStart();

//    if(criticalbuffer-g_TxSize+size>2047)
//        if(g_TxSize+size>2047)
//            RxStart();
    return 0;
}


void Write(uint32_t dwCount, const char * pSrc)
{
    /*
	if(g_bConfig & 0x10)
		dwCount >>= 1;*/
    uint32_t delay = /*g_bConfig*/ regs[0x05] & 0xF;

    totalcycles += 4 + (dwCount * (g_TimeOut + 2 + (2*delay) + 2));



    if(dwCount > 0x10000)
    {
        Write(0x10000, pSrc);
        Write(dwCount-0x10000, pSrc+((/*g_bConfig*/regs[0x05] & 0x10)?0x20000:0x10000));
    }
//	while(dwCount > 0xFFFF)
//	{
//		g_TxBuffer[g_TxSize++] = 0x03;
//		g_TxBuffer[g_TxSize++] = (char) 0xFF;
//		g_TxBuffer[g_TxSize++] = (char) 0xFF;
//		memcpy(g_TxBuffer+g_TxSize, pSrc, (g_bConfig & 0x10)?0x1FFFE:0xFFFF);
//		pSrc += (g_bConfig & 0x10)?0x1FFFE:0xFFFF;
//		g_TxSize += (g_bConfig & 0x10)?0x1FFFE:0xFFFF;
//		dwCount -= 0xFFFF;
//	}
    else
    {
        if(regs[0x05] & 0x40)
            criticalbuffer += (dwCount * regs[0x0C])/32;

        SetCounter(dwCount-1);
        EnqueueCommand(0x03, 0, 0, (uint8_t *) pSrc, (/*g_bConfig*/regs[0x05] & 0x10)?(dwCount<<1):dwCount, 0,0,0);
        regs[0x01] = 0;
        regs[0x02] += (regs[0x02] & (1<<31))?dwCount:0;
//        TxStartThreshold(3+(g_bConfig & 0x10)?(dwCount<<1):dwCount);
//	g_TxBuffer[g_TxSize++] = 0x03;
//	g_TxBuffer[g_TxSize++] = (char) (dwCount >> 0);
//	g_TxBuffer[g_TxSize++] = (char) (dwCount >> 8);
//	memcpy(g_TxBuffer+g_TxSize, pSrc, (g_bConfig & 0x10)?(dwCount<<1):dwCount);
//	g_TxSize += (g_bConfig & 0x10)?(dwCount<<1):dwCount;
    }
}

void WriteToAddress(uint32_t dwAddress, uint16_t wData)
{
	//printf("WriteToAddress(%08x, %04x)\n", dwAddress, wData); fflush(stdout);
    char data[2];
    data[0] = (wData >> 0) & 0xFF;
    if(/*g_bConfig*/regs[0x05] & 0x10)
        data[1] = (wData >> 8) & 0xFF;

        SetAddress(dwAddress, 0);
//        SetOutputs(NOR_DEFAULT_OUT | NOR_CE);
        SetOutputs(NOR_DEFAULT_OUT);
        SetDirections(NOR_DEFAULT_DIR);
        Write(1, data);
//        g_TxBuffer[g_TxSize++] = 0x03;
//        g_TxBuffer[g_TxSize++] = 0x01;
//        g_TxBuffer[g_TxSize++] = 0x00;
//        g_TxBuffer[g_TxSize++] = (wData >> 0) & 0xFF;
//        if(g_bConfig & 0x10)
//                g_TxBuffer[g_TxSize++] = (wData >> 8) & 0xFF;

}

void ReadFromAddress(uint32_t dwAddress, uint16_t *pwData)
{
	SetAddress(dwAddress, 0);
	Read(1, (char *) pwData);
}

void SetData(uint16_t wData)
{
    TxStartThreshold((/*g_bConfig*/regs[0x05] & 0x10)?2:1);

    g_TxBuffer[g_TxSize++] = 0x03;
    g_TxBuffer[g_TxSize++] = 0x01;
    g_TxBuffer[g_TxSize++] = 0x00;
    g_TxBuffer[g_TxSize++] = (wData >> 0) & 0xFF;
    if(/*g_bConfig*/regs[0x05] & 0x10)
            g_TxBuffer[g_TxSize++] = (wData >> 8) & 0xFF;
}

void Stb(uint8_t bValue, uint8_t bMirror)
{
    Write(1, (char *) &bValue);
    /*	g_TxBuffer[g_TxSize++] = 0x03;
	g_TxBuffer[g_TxSize++] = 0x01;
	g_TxBuffer[g_TxSize++] = 0x00;
	g_TxBuffer[g_TxSize++] = bValue;
        if(g_bConfig & 0x10) {
		if(bMirror)
			g_TxBuffer[g_TxSize++] = bValue;
		else
			g_TxBuffer[g_TxSize++] = 0x00;
        }*/
}


void Read(uint32_t dwCount, char * pbyDestination)
{
	//printf("Read(%d, %08x)\n", dwCount, pbyDestination); fflush(stdout);
uint32_t delay = /*g_bConfig*/regs[0x05] & 0xF;

totalcycles += 4 + (dwCount * (g_TimeOut + 2 + (2*delay) + 2));

        uint32_t dwTemp = dwCount;
	
        if(dwCount<=0x10000)
        {
            if(regs[0x05] & 0x40)
                criticalbuffer += (dwCount * regs[0x0C])/32;

            if(regs[0x02] & (1<<31))
            {
                uint32_t address = regs[0x02];
                uint32_t saddr = address &~ 0xFFFF;

                if(((dwCount+address) &~ 0xFFFF) != saddr)
                {   /* address bus only has 16 bit incrementer now */
                    uint32_t count0 = saddr+0x10000-address;
                    uint32_t count1 = dwCount-count0;
                    Read(count0, pbyDestination);
                    SetAddress(saddr+0x10000,1);
                    Read(count1, pbyDestination+((regs[0x05] & 0x10)?(count1<<1):(count1)));
                }
            }
            else
            {
                SetCounter(dwCount-1);
                EnqueueCommand(0x04, 0, 0, 0, 0, (uint8_t*) pbyDestination, (regs[0x05] & 0x10)?dwCount<<1:dwCount, 0);
                regs[0x01] = 0;
                regs[0x02] += (regs[0x02] & (1<<31))?dwCount:0; // update address
            }
        }
        else
        {
            Read(0x10000, pbyDestination);
            if(/*g_bConfig*/regs[0x05] & 0x10)
                Read(dwCount-0x10000, pbyDestination+0x20000);
            else
                Read(dwCount-0x10000, pbyDestination+0x10000);
        }

}

void SetDirections(uint16_t wDirections)
{
    if(regs[0x07] != wDirections)
        EnqueueCommand(0x07, wDirections, 2, 0, 0, 0, 0, 0);
}

void Nop(uint16_t Amount)
{
    SetCounter(Amount);
    EnqueueCommand(0x09, 0, 0, 0, 0, 0, 0, 1);
    regs[0x01] = 0;
    criticalbuffer += Amount/32;
}

void SetAddress(uint32_t dwAddress, uint8_t bAutoIncrement)
{
        if(bAutoIncrement)
                dwAddress |= (1 << 31);
        if(regs[0x02] != dwAddress)
            EnqueueCommand(0x02, dwAddress, 4, 0, 0, 0, 0, 0);

}

void SetOutputs(uint16_t wOutputs)
{
    if(regs[0x06] != wOutputs)
        EnqueueCommand(0x06, wOutputs, 2, 0, 0, 0, 0, 0);
}


void DataPolling(uint8_t ToggleMask, uint8_t TimeOutMask, uint8_t TimeOutValue)
{
    uint32_t temp = TimeOutValue;
    temp <<= 8;
    temp |= TimeOutMask;
    temp <<= 8;
    temp |= ToggleMask;

    EnqueueCommand(0x0D, temp, 3, 0, 0, 0, 0, 1);
}

void WaitRdy(uint8_t High, uint32_t timeout)
{
    criticalbuffer+=timeout/600;
    while(timeout)
    {
//        PrechargeRdy();
//        Delay_ns(100);




        SetTimeOut((timeout>0xFFFF)?0xFFFF:timeout);
//        if(High==0)
//            PrechargeRdy();
        Delay_ns(100);
        EnqueueCommand(High?0x11:0x10, 0, 0, 0, 0, 0, 0, 1);
        timeout-= ((timeout>0xFFFF)?0xFFFF:timeout);

    }

}

void PrechargeRdy()
{
    EnqueueCommand(0x12, 0, 0, 0, 0, 0, 0, 1);
}

void SetTimeOut(uint32_t Value) // in ns
{
    TxStartThreshold(3);
    Value += 20;
    Value /= 21;

    if(regs[0x0C] != Value)
        EnqueueCommand(0x0C, Value>0xFFFF?0xFFFF:Value, 2, 0, 0, 0, 0, 0);
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
                NAND_CE_A = NAND1_CE_A;
                NAND_CE_B = NAND1_CE_B;
	}
	else
	{
		NAND_RDY = NAND0_RDY;
		NAND_CLE = NAND0_CLE;
		NAND_ALE = NAND0_ALE;
		NAND_WP = NAND0_WP;
                NAND_CE_A = NAND0_CE_A;
                NAND_CE_B = NAND0_CE_B;
	}

        if(regs[0x05] != temp)
            EnqueueCommand(0x05, temp, 1, 0, 0, 0, 0, bWaitReady);
}

void SetPinout(uint8_t type)
{
    EnqueueCommand(type?0x0B:0x0A, 0, 0, 0, 0, 0, 0, 0);
    totalcycles += 2;
}

void NAND_ReadID(char ** pbyDest, uint8_t byCount)
{

    char *buf[4];

    for(int j=0; j<4; j++)
    {
        buf[j]=*(pbyDest+j);
    }

    for(int j=0; j<4; j++)
    {
        NAND_SelectChip(j);
        SetOutputs(NAND_CE_LOW | NAND_CLE);
	Stb(0x90, 0);
        SetOutputs(NAND_CE_LOW | NAND_ALE);
	Stb(0, 0);
        SetOutputs(NAND_CE_LOW);
        Read(byCount, buf[j]);
        RxStart();
    }
}




void NAND_AddReadStatus(char * pbyStatus, uint8_t bPrimary)
{
        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_CLE | NAND_WP);
	Stb(0x70, 0);
	Read(1, pbyStatus);
}

//void NAND_ReadBlock(uint32_t dwBlock, char * pbDest, uint8_t bChip, uint8_t bPrimary)
//{
//	NAND_Settings *m_NAND_Settings;
//	//printf(">>>>>>>>>>> in NAND_ReadBlock(%d, pbDest, %d\n", dwBlock, bChip);
//	if(bChip)
//		m_NAND_Settings = &g_NAND1_Settings;
//	else
//		m_NAND_Settings = &g_NAND0_Settings;

//	//printf("Settings: dwBlockSize = %d LargePage = %d BlockCount = %d PageSize = %d\n", m_NAND_Settings->dwBlockSize, m_NAND_Settings->bLargePage, m_NAND_Settings->dwBlockCount, m_NAND_Settings->wPageSize);
//	SetConfig(5, 0, 0, 0, bChip);
//        SetDirections(NAND_CLE | NAND_ALE | NAND_WP | NAND_CE_A | NAND_CE_B);
//        SetOutputs(NAND_CE_A | NAND_CE_B | NAND_WP);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP);

//	uint32_t dwPage = dwBlock * m_NAND_Settings->dwBlockSize;
//	//printf("dwPage = %d\n", dwPage);

//	for(uint32_t i=0; i<m_NAND_Settings->dwBlockSize; i++)
//	{
//		//printf("page %d/%d\n", i, m_NAND_Settings->dwBlockSize);
//                SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP | NAND_CLE);
//		Stb(0, 0);
//                SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP | NAND_ALE);
//		if(m_NAND_Settings->bLargePage)
//		{
//			Stb((uint8_t) 0x00, 0);
//			Stb((uint8_t) 0x00, 0);
//			Stb((uint8_t) ((dwPage >> 0) & 0xFF), 0);
//			Stb((uint8_t) ((dwPage >> 8) & 0xFF), 0);
//                        if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFF)
//				Stb((uint8_t) ((dwPage >> 16) & 0xFF), 0);
//                        if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFFFF)
//				Stb((char) ((dwPage >> 24) & 0xFF), 0);
//                        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP | NAND_CLE);
//			Stb(0x30, 0);
//		}
//		else
//		{
//			Stb((uint8_t) 0x00, 0);
//			Stb((uint8_t) ((dwPage >> 0) & 0xFF), 0);
//			Stb((uint8_t) ((dwPage >> 8) & 0xFF), 0);
//		}

//                SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP);
//                WaitRdy(0, 10000);
//                WaitRdy(1, 300*1000);
////                Nop(255);
////                if(bChip)
////                {
////                    //SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP | NAND_RDY);
////                    SetTrigger(NAND_RDY, 0);
////                    SetTrigger(NAND_RDY, NAND_RDY);
////                }
////                else
////                {
////                    Delay_us(1);
//                    //SetConfig(15, 0, 0, 1, bChip);
////                }

//		//printf("delay %d\n", m_NAND_Settings->read_page_delay_us);
//                //Delay_us(50);//m_NAND_Settings->read_page_delay_us);


//		//SetConfig(10, 0, 0, 1, bChip);

//		Read(m_NAND_Settings->wPageSize, pbDest);
//                //SetConfig(10, 0, 0, 0, bChip);
//		dwPage++;
//		pbDest += m_NAND_Settings->wPageSize;
//		//if(g_bConfig & 0x10)
//		//	pbDest += m_NAND_Settings->wPageSize;

//	//	SetOutputs(NAND_CE | NAND_WP);
//		//RxConditional();
//                //RxStart();
//		//Nop(5);
//	}
//        SetOutputs(NAND_CE_A | NAND_CE_B | NAND_WP);
//	//SetConfig(10, 0, 1, 0, bChip);
//	RxStart();
//}


void NAND_SelectChip(uint8_t j)
{
    //SetOutputs(NAND_CE_HIGH);
    SetDirections(NAND0_CLE | NAND0_ALE | NAND0_WP | NAND0_CE_A | NAND0_CE_B
                  | NAND1_CLE | NAND1_ALE | NAND1_WP | NAND1_CE_A | NAND1_CE_B);
    switch(j)
    {
        case 0: NAND_CE_LOW = NAND0_CE_B | NAND1_CE_A | NAND1_CE_B | NAND0_WP | NAND1_WP; NAND_CLE = NAND0_CLE; NAND_ALE = NAND0_ALE; break;
        case 1: NAND_CE_LOW = NAND0_CE_A | NAND1_CE_A | NAND1_CE_B | NAND0_WP | NAND1_WP; NAND_CLE = NAND0_CLE; NAND_ALE = NAND0_ALE; break;
        case 2: NAND_CE_LOW = NAND1_CE_B | NAND0_CE_A | NAND0_CE_B | NAND0_WP | NAND1_WP; NAND_CLE = NAND1_CLE; NAND_ALE = NAND1_ALE; break;
        case 3: NAND_CE_LOW = NAND1_CE_A | NAND0_CE_A | NAND0_CE_B | NAND0_WP | NAND1_WP; NAND_CLE = NAND1_CLE; NAND_ALE = NAND1_ALE; break;
        default: break;
    }
    SetConfig(15,0,0,0,j>1);
    //Delay_us(10);
    SetOutputs(NAND_CE_LOW);
    //Delay_us(10);
    WaitRdy(1,10000);
}


void NAND_ReadBlock(uint32_t dwBlock, char ** pbDest, uint32_t dwBlockCount, uint32_t dwBlockSize, uint16_t wPageSize, bool bLargePage)
{
    SetOutputs(NAND_CE_HIGH);
    uint16_t time_start[4] = {0,0,0,0};
    uint16_t time_end[4] = {0,0,0,0};
        uint32_t dwPage = dwBlock * dwBlockSize;
        char *buf[4];

        for(int j=0; j<4; j++)
        {
            buf[j]=*(pbDest+j);
        }

	//printf("dwPage = %d\n", dwPage);

        for(uint32_t i=0; i<dwBlockSize; i++)
	{

            for(int j=0; j<4; j++)
            {
                if(buf[j] != 0)
                {
                    NAND_SelectChip(j);

                    SetOutputs(NAND_CE_LOW | NAND0_WP | NAND1_WP | NAND_CLE);
                    Stb(0, 0);
                    SetOutputs(NAND_CE_LOW | NAND0_WP | NAND1_WP | NAND_ALE);
                    if(bLargePage)
                    {
                            Stb((uint8_t) 0x00, 0);
                            Stb((uint8_t) 0x00, 0);
                            Stb((uint8_t) ((dwPage >> 0) & 0xFF), 0);
                            Stb((uint8_t) ((dwPage >> 8) & 0xFF), 0);
                            if((dwBlockCount * dwBlockSize) > 0xFFFF)
                                    Stb((uint8_t) ((dwPage >> 16) & 0xFF), 0);
                            if((dwBlockCount * dwBlockSize) > 0xFFFFFF)
                                    Stb((char) ((dwPage >> 24) & 0xFF), 0);
                            SetOutputs(NAND_CE_LOW | NAND0_WP | NAND1_WP | NAND_CLE);
                            Stb(0x30, 0);
                    }
                    else
                    {
                            Stb((uint8_t) 0x00, 0);
                            Stb((uint8_t) ((dwPage >> 0) & 0xFF), 0);
                            Stb((uint8_t) ((dwPage >> 8) & 0xFF), 0);
                    }
                    //WaitRdy(1, 10000);
                    SetOutputs(NAND_CE_LOW | NAND0_WP | NAND1_WP);
                    WaitRdy(0, 10000);
                    GetTimeBase(&time_start[j]);
//                }
//            }

//            for(int j=0; j<4; j++)
//            {
//                if(buf[j])
//                {

//                    NAND_SelectChip(j);
                    WaitRdy(1, 300*1000); //Delay_us(300);
                    GetTimeBase(&time_end[j]);
                    Read(wPageSize, buf[j]+(i*wPageSize));
                    //buf[j] += wPageSize;
                }
            }
            dwPage++;
        }
        SetOutputs(NAND_CE_HIGH);
	//SetConfig(10, 0, 1, 0, bChip);
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


void NAND_EraseBlock(uint32_t dwBlockNumber, uint32_t dwBlockSize, uint32_t dwBlockCount, bool *enabled, char *status)
{
    SetOutputs(NAND_CE_HIGH);

    uint16_t time_start[4];
    uint16_t time_end[4];

    dwBlockNumber *= dwBlockSize;

    for(int j=0; j<4; j++)
    {
        if(*(enabled+j))
        {
            NAND_SelectChip(j);
            SetOutputs(NAND_CE_LOW | NAND_CLE);
            Stb(0x60, 0);
            SetOutputs(NAND_CE_LOW | NAND_ALE);

            Stb((dwBlockNumber>>0) & 0xFF, 0);
            Stb((dwBlockNumber>>8) & 0xFF, 0);
            if((dwBlockCount) > 0xFFFF)
                    Stb((dwBlockNumber>>16) & 0xFF, 0);
            if((dwBlockCount) > 0xFFFFFF)
                    Stb((dwBlockNumber>>24) & 0xFF, 0);
            SetOutputs(NAND_CE_LOW | NAND_CLE);
            Stb(0xD0, 0);
            SetOutputs(NAND_CE_LOW);
            WaitRdy(0, 100000);
//            SetOutputs(NAND_CE_HIGH);
//            GetTimeBase(&time_start[j]);
//        }
//    }

//    for(int j=0; j<4; j++)
//    {
//        if(*(enabled+j))
//        {
//            NAND_SelectChip(j);
            WaitRdy(1, 3000*1000);
            GetTimeBase(&time_end[j]);
            SetOutputs(NAND_CE_LOW | NAND_CLE);
            Stb(0x70, 0);
            Delay_us(10);
            SetOutputs(NAND_CE_LOW);
            Read(1, status+j);
        }
    }

    SetOutputs(NAND_CE_HIGH);
    //SetConfig(10, 0, 1, 0, bChip);
    RxStart();
}



//char NAND_EraseBlock(uint32_t dwBlockNumber, uint8_t bChip, uint8_t bPrimary)
//{
//	NAND_Settings *m_NAND_Settings;
//	if(bChip)
//		m_NAND_Settings = &g_NAND1_Settings;
//	else
//		m_NAND_Settings = &g_NAND0_Settings;

//	SetConfig(10, 0, 0, 0, bChip);
//        SetDirections(NAND_CE_A | NAND_CE_B | NAND_CLE | NAND_ALE | NAND_WP | NAND_CE);

//        SetOutputs(NAND_CE_A | NAND_CE_B | NAND_WP);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_CLE | NAND_WP);
//	Stb(0x60, 0);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_ALE | NAND_WP);
//	dwBlockNumber *= m_NAND_Settings->dwBlockSize;
//	Stb((dwBlockNumber>>0) & 0xFF, 0);
//	Stb((dwBlockNumber>>8) & 0xFF, 0);
//	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFF)
//		Stb((dwBlockNumber>>16) & 0xFF, 0);
//	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFFFF)
//		Stb((dwBlockNumber>>24) & 0xFF, 0);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_CLE | NAND_WP);
//	Stb(0xD0, 0);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP);
//        WaitRdy(0, 10000); //Delay_ms(1);
//        WaitRdy(1, 3000000);
////        if(bChip)
////        {
////            SetTrigger(NAND_RDY, 0);
////            SetTrigger(NAND_RDY, NAND_RDY);
////        }
//        SetConfig(10, 0, 0, 0, bChip);
//        //Delay_ms(3);

//	char status = 0;
//        NAND_AddReadStatus(&status, bPrimary);
//	RxStart();

//	return status;
//}

//char NAND_ProgramPage(uint32_t dwPage, char * pbyData, uint8_t bChip, uint8_t bPrimary)
//{
//	NAND_Settings *m_NAND_Settings;
//	if(bChip)
//		m_NAND_Settings = &g_NAND1_Settings;
//	else
//		m_NAND_Settings = &g_NAND0_Settings;

//	//SetOutputs(NAND_CE | NAND_WP);
//        SetConfig(10, 0, 0, 0, bChip);

//	uint16_t wOffset = 0;
//	while(pbyData[wOffset] == (char)0xFF)
//		wOffset++;

//	uint16_t wEnd = m_NAND_Settings->wPageSize-1;
//	while(pbyData[wEnd] == (char)0xFF)
//		wEnd--;
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_CLE | NAND_WP);
//	Stb(0x80, 0);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_ALE | NAND_WP);
//	Stb((wOffset >> 0) & 0xFF, 0);
//	if(m_NAND_Settings->bLargePage)
//		Stb((wOffset >> 8) & 0xFF, 0);
//	Stb((dwPage >> 0) & 0xFF, 0);
//	Stb((dwPage >> 8) & 0xFF, 0);
//	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFF)
//		Stb((dwPage >> 16) & 0xFF, 0);
//	if((m_NAND_Settings->dwBlockCount * m_NAND_Settings->dwBlockSize) > 0xFFFFFF)
//		Stb((dwPage >> 24) & 0xFF, 0);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP);
//	Write(wEnd-wOffset+1, pbyData+wOffset);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP | NAND_CLE);
//	Stb(0x10, 0);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP);
//        //Delay_us(20);
////        if(bChip)
////        {
////            SetTrigger(NAND_RDY, 0);
////            SetTrigger(NAND_RDY, NAND_RDY);
////        }
////        else
//        //{
//            //Delay_us(1);
//        WaitRdy(0, 10000);
//        WaitRdy(1, 800000);
//            SetConfig(10, 0, 0, 0, bChip);
//        //}
//        //Delay_us(800);
//	return 0;
//}

//char NAND_ProgramBlock(uint32_t dwBlock, char * pbyData, uint8_t bChip, uint8_t bPrimary)
//{
//	NAND_Settings *m_NAND_Settings;
//	if(bChip)
//		m_NAND_Settings = &g_NAND1_Settings;
//	else
//		m_NAND_Settings = &g_NAND0_Settings;

//	/*char * pbyActualContent = (char *) malloc(m_NAND_Settings->dwBlockSize * m_NAND_Settings->wPageSize);
//	NAND_ReadBlock(dwBlock, pbyActualContent, bChip);
//	if( memcmp(pbyActualContent, pbyData, m_NAND_Settings->dwBlockSize * m_NAND_Settings->wPageSize) == 0 )
//	{
//		free(pbyActualContent);
//		return 0;
//	}*/


////	SetConfig(10, 0, 0, !bChip, bChip);
//        SetDirections(NAND_CLE | NAND_ALE | NAND_WP | NAND_CE_A | NAND_CE_B);
//        SetOutputs(NAND_WP | NAND_CE_A | NAND_CE_B);
//        SetOutputs((bPrimary?NAND_CE_B:NAND_CE_A) | NAND_WP);

//	char * pbyTemp = (char *) malloc(m_NAND_Settings->wPageSize);
//	memset(pbyTemp, 0xFF, m_NAND_Settings->wPageSize);

//	char * status = (char *) malloc(m_NAND_Settings->dwBlockSize);
//	memset(status, 0, m_NAND_Settings->dwBlockSize);

//	for(uint32_t i=0; i<m_NAND_Settings->dwBlockSize; i++)
//		if(memcmp(pbyData+(i*m_NAND_Settings->wPageSize), pbyTemp, m_NAND_Settings->wPageSize))
//		{
//                        NAND_ProgramPage((dwBlock*m_NAND_Settings->dwBlockSize)+i, pbyData+(i*m_NAND_Settings->wPageSize), bChip, bPrimary);
//                        NAND_AddReadStatus(status+i, bPrimary);
//                        //RxStart();
			
//		}

//        RxStart();
//	char res = 0;
//	for(uint32_t i=0; i<m_NAND_Settings->dwBlockSize; i++)
//		res |= *(char *)(status+i);
//	free(pbyTemp);
//	free(status);
//	return res;// & 0x1;
//}






void NAND_ProgramBlock(uint32_t dwBlock, char ** pbSrc, uint32_t dwBlockCount, uint32_t dwBlockSize, uint16_t wPageSize, bool bLargePage, char *status)
{
    SetOutputs(NAND_CE_HIGH);
    char *statustemp = (char *) malloc(4*dwBlockSize);
    memset(statustemp, 0xE0, 4*dwBlockSize);


    uint16_t time_start[4];
    uint16_t time_end[4];
    bool writing[4];

    uint32_t dwPage = dwBlock * dwBlockSize;
    char *buf[4];

    for(int j=0; j<4; j++)
    {
        buf[j]=*(pbSrc+j);
    }

    //printf("dwPage = %d\n", dwPage);


    char * pbyTemp = (char *) malloc(wPageSize);
    memset(pbyTemp, 0xFF, wPageSize);

    for(uint32_t i=0; i<dwBlockSize; i++)
    {
        for(int j=0; j<4; j++)
        {
            uint16_t wOffset = 0;
            uint16_t wEnd = wPageSize-1;
            if(buf[j] != 0)
            {

                NAND_SelectChip(j);
                writing[j] = false;
                if(memcmp(buf[j]+(i*wPageSize), pbyTemp, wPageSize))
                {
                    writing[j] = true;
                    WaitRdy(1,100000);
                    while((*(buf[j]+(i*wPageSize)+wOffset) == (char)0xFF) && (wOffset<255))
                            wOffset++;
                    while((*(buf[j]+(i*wPageSize)+wEnd) == (char)0xFF)  && (wOffset<wEnd))
                            wEnd--;
                    SetOutputs(NAND_CE_LOW | NAND_CLE);
                    Stb(0x80, 0);
                    SetOutputs(NAND_CE_LOW | NAND_ALE);
                    Stb((wOffset >> 0) & 0xFF, 0);
                    if(bLargePage)
                            Stb((wOffset >> 8) & 0xFF, 0);
                    Stb((dwPage >> 0) & 0xFF, 0);
                    Stb((dwPage >> 8) & 0xFF, 0);
                    if((dwBlockCount * dwBlockSize) > 0xFFFF)
                            Stb((dwPage >> 16) & 0xFF, 0);
                    if((dwBlockCount * dwBlockSize) > 0xFFFFFF)
                            Stb((dwPage >> 24) & 0xFF, 0);
                    SetOutputs(NAND_CE_LOW);
                    Write(wEnd-wOffset+1, buf[j]+(i*wPageSize)+wOffset);
                    SetOutputs(NAND_CE_LOW | NAND_CLE);
                    Stb(0x10, 0);
                    WaitRdy(0, 100000);
                    GetTimeBase(&time_start[j]);

                }
            }
        }



        for(int j=0; j<4; j++)
            if(buf[j] && writing[j])
            {
                NAND_SelectChip(j);
                WaitRdy(1, 800000);
                GetTimeBase(&time_end[j]);
                SetOutputs(NAND_CE_LOW | NAND_CLE);
                Stb(0x70, 0);
                SetOutputs(NAND_CE_LOW);
                Read(1, statustemp+(i*4)+j);
            }
        dwPage++;
    }



    RxStart();
    for(int j=0; j<4; j++)
    {
        unsigned char res = 0xE0;
        unsigned char error = 0;
        if(buf[j])
        {
            for(uint32_t i=0; i<dwBlockSize; i++)
            {
                res &= *(statustemp+(i*4)+j);
                error |= (*(statustemp+(i*4)+j) & 1);
            }
        }
        *(status+j) = res | error;
    }
    free(pbyTemp);
    free(statustemp);
}




void NAND_Configure(uint16_t wPageSize, uint32_t dwBlockSize, uint32_t dwBlockCount, char bLargePage,
								 char bChipSelect, uint32_t read_page_delay_us)
{
    SetPinout(0);
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
//	uint16_t g_result[16];
//	uint16_t d_result[16];
//        uint32_t result = 0;
//	SetConfig(10, 1, 0, 0, 0);

//	for(int i=0; i<16; i++) {
//		WriteToAddress(0, 1<<i);
//		Delay_us(20);
//		Read(1, (char *) &d_result[i]);
//		SetDirections(0xFFFF);
//		SetOutputs(0);
		
//                Nop(255);
//		Nop(255);
//		SetOutputs(1<<i);
//		SetDirections(1<<i);
//		Delay_us(20);
//		SetDirections(0);
//		GetInputs(&g_result[i]);
//	}
//	RxStart();

//	for(int i=0; i<16; i++) {
//		if(d_result[i] != (1<<i))
//                    result |= (1 << i);
//		if(g_result[i] != (1<<i))
//                    result |= (1 << (i+16));
//	}
//        return result;
}

void Nopulate(uint32_t nops)
{
    while (nops) {
        Nop(nops>0x10000?0xFFFF:nops-1);
        nops -= (nops>0x10000)?0x10000:nops;
    }
}

void Delay_ns(uint32_t delay)
{
    static const double ratio = 20.833333333333333333333333333333;
    Nopulate((delay / ratio) + 1); /* Better 1 more than one less */
}

void Delay_us(uint32_t delay)
{
    while(delay)
    {
        Delay_ns(delay>4294967?4294967000:delay*1000);
        delay-=(delay>4294967)?4294967:delay;
    }
}

void Delay_ms(uint32_t delay)
{
    while(delay)
    {
        Delay_us(delay>4294967?4294967000:delay*1000);
        delay-=(delay>4294967)?4294967:delay;
    }
}

int NOR_Configure(struct CommonArgs common, struct NorArgs nor)
{
    g_Common_Settings = common;
    g_Common_Settings.period = 15;
    g_NOR_Settings = nor;

    /* Statically configure the timings, needs configuration */
//    g_NOR_Settings.timings.tBUSY = 90; /* ns */
//    g_NOR_Settings.timings.tRB = 50; /* ns, only needed for Macronix */
//    g_NOR_Settings.timings.tWHWH1_BufWord = 64; /* us, max = 2048us for 64 byte buffer */
//    g_NOR_Settings.timings.tWHWH1_Word = 64;

    SetPinout(1);
    SetDirections(NOR_DEFAULT_DIR);
    SetOutputs(NOR_DEFAULT_OUT | NOR_CE);
    Delay_us(100);
    SetOutputs(NOR_DEFAULT_OUT);
    return RxStart();
}

int NOR_Read(char* buf, uint32_t addr, uint32_t len)
{
    //printf("NOR_Read start\n"); fflush(stdout);
    addr >>= g_Common_Settings.word;
    len >>= g_Common_Settings.word;

    //SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, 0);
    //WriteToAddress(0x00, 0x0F); Nop(255);
    //PrechargeRdy();
    SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 0, g_Common_Settings.byteSwap);
    SetAddress(addr, 1); //DataPolling(0xFF, 0, 0);
    Read(len, buf);

    return 0;//RxStart();
}

void NOR_Wait(uint32_t addr, uint32_t lenw)
{
    char buf[2];

    switch (g_NOR_Settings.waitMethod) {
    case NorWaitMethod_RdyTrigger:
//        /* Charge cap for trigger */
//        SetDirections(NOR_DEFAULT_DIR | NOR_ALT_RDY);
//        SetOutputs(NOR_DEFAULT_OUT | NOR_ALT_RDY);

//        /* tBUSY */
//        //Delay_ns(g_NOR_Settings.timings.tBUSY);
//        Delay_us(5);

//        /* Trigger wait for NOR RDY (tWHWH1) */
//        SetDirections(NOR_DEFAULT_DIR); Nop(255);
//        SetTrigger(NOR_ALT_RDY, NOR_ALT_RDY);
        SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, 1, g_Common_Settings.byteSwap);
        break;
    case NorWaitMethod_DataPolling:
        /* tBUSY */
        Delay_ns(g_NOR_Settings.timings.tBUSY);

        SetAddress(addr + lenw - 1, 0);
        SetConfig(10, 0, 0, 1, 0);

        /* Poll data */
        buf[0] = 0xFF;
        buf[1] = 0x00;
        do {
            Read(2, buf);
            RxStart();
        } while(buf[0] != buf[1]);

        SetConfig(10, g_Common_Settings.word, 0, 1, 0);
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
        RxStart();
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
}

void NOR_BufferWrite(uint32_t base, uint32_t offset, const char* data, uint32_t len)
{
    uint32_t mask = g_NOR_Settings.bufferedMax - 1;
    uint32_t writtenwords = 0;

    for(int i=0; i<len; i+=2)
        if((*(data+i) != 0xFF) || (*(data+i+1) != 0xFF))
            writtenwords++;

    DWORD saddr = (offset &~ mask) >> g_Common_Settings.word; //0x1F;

    SetConfig(15, 1, 0, 0, 0);
    //RxStart();
    WriteToAddress(0x555 | base, 0xAA);
    //RxStart();
    WriteToAddress(0x2AA | base, 0x55);
    //RxStart();
    WriteToAddress(saddr, 0x25);
    //RxStart();
    WriteToAddress(saddr, (len>>g_Common_Settings.word)-1);
    //RxStart();
    SetAddress((offset >> g_Common_Settings.word), 1);
    //RxStart();
    Nop(255);
    //RxStart();
    SetConfig(15, 1, 0, 0, g_Common_Settings.byteSwap);
    //RxStart();
    Write((len>>g_Common_Settings.word), data);
    //RxStart();
    SetConfig(15, 1, 0, 0, 0);
    //RxStart();
    WriteToAddress(saddr, 0x29);
    //

    //PrechargeRdy();
    WaitRdy(0, 10000);
    //WaitRdy(1, g_NOR_Settings.timings.tWHWH1_BufWord*1000*(len>>g_Common_Settings.word));
    DataPolling((1<<6), (1<<5), 0);
    //PrechargeRdy();
    //WaitRdy(1);
    //Nop(255);
    //Delay_us(g_NOR_Settings.timings.tWHWH1_BufWord);

}

void NOR_Program(uint32_t base /* address of chip */, uint32_t offset, const char* data, uint32_t len, const char* currentdata) // addr in bytes, len in bytes
{
    uint32_t buf_max, written, addrw;


    base >>= g_Common_Settings.word;
    //len >>= g_Common_Settings.word;


    buf_max = g_NOR_Settings.bufferedMax;
    uint8_t *ff_buf = (uint8_t *) malloc(64);//g_NOR_Settings.bufferedMax);
    memset(ff_buf, 0xFF, buf_max);
    written = 0;

   // DebugWrite("NOR_Program(addr=0x%04x, data=%p, len=%u)", addr, data, len);

    SetConfig(g_Common_Settings.period, g_Common_Settings.word, 0, /*(g_NOR_Settings.waitMethod == NorWaitMethod_RdyTrigger)*/0, 0);
    SetDirections(NOR_DEFAULT_DIR);
    SetOutputs(NOR_DEFAULT_OUT | NOR_CE);
    Delay_us(1);
    SetOutputs(NOR_DEFAULT_OUT);

    switch (g_NOR_Settings.writeMethod) {
    case NorWriteMethod_BufferedWrite:



        while ((len - written) > buf_max) {
            if(memcmp(ff_buf, data+written, buf_max))
                NOR_BufferWrite(base, offset+written, data + written, buf_max);
            written += buf_max;
        }

        if (len - written){

            if(memcmp(ff_buf, data+written, len-written))
                NOR_BufferWrite(base, offset+written, data + written, len-written);
        }
        break;

    case NorWriteMethod_SingleWordProgram:
        SetTimeOut(g_NOR_Settings.timings.tWHWH1_Word*1000);
        while (len - written) {
            uint8_t byte0, byte1;
            uint16_t wordtowrite, currentlywrittenword;


            byte0 = *(data+written);
            byte1 = 0xFF;
            if(g_Common_Settings.word) {
                byte1 = *(data+written+1);
            }

            wordtowrite = *(uint16_t*)(data + written); //(((uint16_t) byte1) << 8) | byte0;
            currentlywrittenword = *(uint16_t*)(currentdata + written);
            if(currentlywrittenword != wordtowrite)
            {

                //DataPolling((1 << 2), (1<<5), (1<<5));
//                SetOutputs(NOR_DEFAULT_OUT | NOR_CE);
//                Delay_us(1);
//                SetOutputs(NOR_DEFAULT_OUT);
//                Delay_us(1);
                SetConfig(g_Common_Settings.period, 1, 0, 1/*(g_NOR_Settings.waitMethod == NorWaitMethod_RdyTrigger)*/, 0);
                addrw = (offset + written) >> g_Common_Settings.word;
                WriteToAddress(base | 0x555, 0xAA); // adjust for byte
                WriteToAddress(base | 0x2AA, 0x55); // same here
                WriteToAddress(base | 0x555, 0xA0); // same here
                SetConfig(g_Common_Settings.period, 1, 0, 1/*(g_NOR_Settings.waitMethod == NorWaitMethod_RdyTrigger)*/, g_Common_Settings.byteSwap);
                            //WriteToAddress(addrw, *(uint16_t*)(data + written));
                WriteToAddress(addrw, wordtowrite);
                WaitRdy(0, 10000);
               // WaitRdy(1, g_NOR_Settings.timings.tWHWH1_Word*1000);
                //Delay_us(g_NOR_Settings.timings.tWHWH1_Word);
                //DataPolling(0xFF, 0xFF, 0xFF);
//                if(g_NOR_Settings.waitMethod != NorWaitMethod_RdyTrigger)
//                    NOR_Wait(addrw, 1);

                //Delay_us(20);

                //SetOutputs(NOR_DEFAULT_OUT);

                //DataPolling((1<<6), 0xFF, g_Common_Settings.byteSwap?byte1:byte0);
//                uint8_t dq7_n;
//                dq7_n = (g_Common_Settings.byteSwap?byte1:byte0) & (1<<7);
//                dq7_n ^= (1<<7);
//                DataPolling((1<<6), (1<<7) | (1<<5) | (1<<1), dq7_n);

                //Delay_us(10);
                //TxStart();
                //TxStartThreshold();
            }
            written += (1 << g_Common_Settings.word);
        }
        break;

    case NorWriteMethod_DoubleWordProgram:
        while (len - written) {
//            addrw = (addr + written);

//            WriteToAddress(0x555, 0x50);

//            SetConfig(10, g_Common_Settings.word, 0, 0, g_Common_Settings.byteSwap);
//            WriteToAddress(addrw, *(uint16_t*)(data + written));
//            WriteToAddress(addrw + (2 >> g_Common_Settings.word), *(uint16_t*)(data + written + 2));
//            SetConfig(10, g_Common_Settings.word, 0, 0, 0);

//            NOR_Wait(addrw, 2);

//            TxStartThreshold();

//            written += 4;
        }
        break;
    default:
        break;
    }
free(ff_buf);
    SetConfig(10, 0, 0, 1, 0);
    //RxStart();

//    char buf[2];
//    buf[0] = 0xFF;
//    buf[1] = 0x00;
//    do {
//        Read(2, buf);
//        RxStart();
//    } while(buf[0] != buf[1]);

   // SetOutputs(NOR_DEFAULT_OUT | NOR_CE);

    SetConfig(10, g_Common_Settings.word, 0, 0, 0);

    //RxStart();
//    free(ff_buf);

    DebugWrite("NOR_Program finished");
}

uint8_t NOR_Erase(uint32_t addr, uint32_t base)
{
    uint8_t res = 0;
    SetDirections(NOR_DEFAULT_DIR);
    SetOutputs(NOR_DEFAULT_OUT | NOR_CE);
    Delay_us(1);
    SetOutputs(NOR_DEFAULT_OUT);
    addr >>= g_Common_Settings.word;
    SetConfig(g_Common_Settings.period, 1, 0, 0, 0);
    WriteToAddress(0x555 | base, 0xAA);
    WriteToAddress(0x2AA | base, 0x55);
    WriteToAddress(0x555 | base, 0x80);
    WriteToAddress(0x555 | base, 0xAA);
    WriteToAddress(0x2AA | base, 0x55);
    WriteToAddress(addr, 0x30);
    //SetAddress(addr,0);
    //Delay_us(100);
    //DataPolling((1<<6), 0xFF, 0xFF);
    DataPolling((1<<6) | (1<<2), (1<<7) | (1<<5) | (1<<3), 0); //time-out period
    DataPolling((1<<6) | (1<<2), (1<<7) | (1<<5) | (1<<3), (1<<3)); // in progress
    //TxStart();
//    SetConfig(15, 0, 0, 1, 0);
//    Read(1, &res);
//    RxStart();





    SetConfig(15, 0, 0, 0, 0);
    SetAddress(addr, 0);
    char buf[5];
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;

    bool success = false;

    do {
        Read(2, buf);
        RxStart();
//        if( (buf[4] & (1 << 5)) || ((buf[0] & (1 << 6)) == (buf[1] & (1 << 6))))
//            success = true;
    } while(buf[0] != buf[1]);

    return buf[0];
}

void NOR_PPBErase()
{
    /* FIXME ? */
    WriteToAddress(0xFF, 0x80);
    WriteToAddress(0, 30);
    RxStart();
}

int NOR_Reset()
{
    SetConfig(10, 1, 0, 0, 0);

    SetDirections(NOR_DEFAULT_DIR);

    uint16_t out = NOR_DEFAULT_OUT;

    out |= NOR_CE; /* CE -> HIGH */
    SetOutputs(out); Nop(255);

    out &= ~NOR_CE; /* CE -> LOW */
    SetOutputs(out); Nop(255);

    out &= ~NOR_RST; /* RST -> LOW */
    SetOutputs(out); Delay_ms(2);

    out |= NOR_RST; /* RST -> HIGH */
    SetOutputs(out); Delay_ms(2);

    SetConfig(10, 1, 0, 1, 0);

    return RxStart();
}

#define CFI_DUMP_SIZE 0x100

int NOR_ReadCFI(char* buf, uint32_t len)
{
    if (len < CFI_DUMP_SIZE / 2)
        return 1;

    memset(buf, 0, len);

   // PrechargeRdy();

    SetConfig(10, 0, 0, 0, 0);
    SetDirections(NOR_DEFAULT_DIR);
    SetOutputs(NOR_DEFAULT_OUT | NOR_CE);
    SetOutputs(NOR_DEFAULT_OUT);

    /* Exit CFI query */
    WriteToAddress(0x00, 0x0F);
    /* Start CFI query */
    WriteToAddress(0x55, 0x98);

    /* Start at 0x20 (8-bit) / 0x10 (16-bit) */
    SetAddress(0x20 >> g_Common_Settings.word, 1);

    WaitRdy(0, 1000);
    WaitRdy(1,10000);
    DataPolling(0xFF, 0, 0);
    //PrechargeRdy();
    /* Read actual data */
    Read(CFI_DUMP_SIZE >> g_Common_Settings.word, buf);

    /* Exit CFI query */
    WriteToAddress(0x00, 0x0F);

    return RxStart();

}



void SPI_Write(uint32_t count, uint8_t *src)
{
    SetCounter(count-1);
    EnqueueCommand(0x0E, 0, 0, src, count, 0,0,0);
    regs[0x01] = 0;
}

void SPI_Write8(uint8_t data)
{
    SPI_Write(1, (uint8_t*)&data);
}

void SPI_Write16(uint16_t data)
{
    SPI_Write(2, (uint8_t*) &data);
}

void SPI_Write32(uint32_t data)
{
    SPI_Write(4, (uint8_t*)&data);
}


void SPI_Read(uint32_t count, uint8_t *dest)
{
    SetCounter(count-1);
    EnqueueCommand(0x0F, 0, 0, 0, 0, (uint8_t*)dest, count, 0);
    regs[0x01] = 0;
}

void SPI_Read8(uint8_t *dest)
{
    SPI_Read(1, dest);
}

void SPI_Read16(uint8_t *dest)
{
    SPI_Read(2, dest);
}

void SPI_Read32(uint8_t *dest)
{
    SPI_Read(4, dest);
}

void SF_Read(uint32_t address, uint32_t len, uint8_t *dest)
{
    uint32_t temp;
    SetDirections(SPI_CS);
    SetOutputs(SPI_CS);
    SetOutputs(0);

    for(int i=0; i<3; i++)
    {
        temp |= (address >> 0) & 0xFF;
        temp <<= 8;
        address >>= 8;
    }
    temp |= 0x03;

    SPI_Write32(temp);
    SPI_Read(len, dest);
    SetOutputs(SPI_CS);
}


void SF_Program_Page(uint32_t address, uint32_t len, uint8_t *src)
{
    uint32_t temp;
    SetOutputs(SPI_CS);
    SetOutputs(0);
    SPI_Write8(0x06);
    SetOutputs(SPI_CS);

    SetOutputs(0);

    for(int i=0; i<3; i++)
    {
        temp |= (address >> 0) & 0xFF;
        temp <<= 8;
        address >>= 8;
    }
    temp |= 0x02;


    SPI_Write32(temp);
    SPI_Write(len, src);

    SetOutputs(SPI_CS);
    RxStart();
}

void SF_ReadID(uint8_t *dest)
{
    uint32_t temp;
    SetConfig(15, 0, 0, 0, 0);
    SetDirections(SPI_CS);
    SetOutputs(SPI_CS);
    SetOutputs(0);
    SPI_Write8(0x9F);

    SPI_Read16(dest);

    SetOutputs(SPI_CS);
    RxStart();
}


void Glitch_Set(uint16_t start, uint16_t end)
{
    g_TxBuffer[g_TxSize++] = start&0xFF;
    g_TxBuffer[g_TxSize++] = (start>>8)&0xFF;

    g_TxBuffer[g_TxSize++] = end&0xFF;
    g_TxBuffer[g_TxSize++] = (end>>8)&0xFF;
    g_RxSize += 3;
    RxStart();
}

void Glitch_Wait()
{
    g_RxSize++;
    RxStart();
}
