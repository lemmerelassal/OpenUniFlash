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

#include "progskeet.h"

#ifndef WIN32
#ifdef __APPLE__
#include <libusb-legacy/usb.h>
#else /* !__APPLE__ */
#include <usb.h>
#endif /* __APPLE__ */
#else /* WIN32 */
#include <lusb0_usb.h>
#endif /* !WIN32 */

/* usblib helpers */
#define USB_HANDLE(x) ((struct usb_dev_handle*)x->hdev)

/* USB defines */
#define PROGSKEET_USB_VID 0x1988
#define PROGSKEET_USB_PID 0x0001

#define PROGSKEET_USB_CFG 1
#define PROGSKEET_USB_INT 0

#define PROGSKEET_USB_EP_OUT 0x01
#define PROGSKEET_USB_EP_IN 0x82
#define PROGSKEET_USB_EP_CONTROL 0x03

#define PROGSKEET_USB_TIMEOUT 1000 /* 1 second timeout for USB transfers */

/* Other defines */
#define PROGSKEET_TXBUF_LEN (2 ^ 11) /* 2 KiB */

#define PROGSKEET_ADDR_AUTO_INC (1 << 23)

/* Command codes */
#define PROGSKEET_CMD_GET_GPIO      0x01
#define PROGSKEET_CMD_SET_ADDR      0x02
#define PROGSKEET_CMD_WRITE_CYCLE   0x03
#define PROGSKEET_CMD_READ_CYCLE    0x04
#define PROGSKEET_CMD_SET_CONFIG    0x05
#define PROGSKEET_CMD_SET_GPIO      0x06
#define PROGSKEET_CMD_SET_GPIO_DIR  0x07
#define PROGSKEET_CMD_WAIT_GPIO     0x08
#define PROGSKEET_CMD_NOP           0x09

/* Configuration (PROGSKEET_CMD_SET_CONFIG) */
#define PROGSKEET_CFG_DELAY_MASK    0x0F
#define PROGSKEET_CFG_WORD          (1 << 4)
#define PROGSKEET_CFG_TRISTATE      (1 << 5)
#define PROGSKEET_CFG_WAIT_RDY      (1 << 6)
#define PROGSKEET_CFG_BYTESWAP      (1 << 7)

struct progskeet_rxloc
{
    char* addr;
    size_t len;

    struct progskeet_rxloc* next;
};

struct progskeet_handle
{
    /* LibUSB device handle */
    void* hdev;

    /* Transmit buffers */
    char* txbuf;
    size_t txlen;

    /* Receive list */
    struct progskeet_rxloc* rxlist;

    /* Set to 1 to cancel any running operations */
    int cancel;

    uint32_t cur_addr;
    uint16_t cur_gpio;
    uint16_t cur_gpio_dir;
    uint8_t cur_config;

    /*
     * This gets used to mask and add to the address.
     * It's used for such things as the virtual chip enable
     * on Samsung K8Q.
     */
    uint32_t addr_mask;
    uint32_t addr_add;
};

int progskeet_free_rxlist(struct progskeet_rxloc* list)
{
    struct progskeet_rxloc* next;

    while (list) {
        next = list->next;
        free(list);
        list = next;
    }

    return 0;
}

int progskeet_init()
{
    static int inited = 0;

    if (inited == 0) {
        usb_init();
        usb_set_debug(255);

        inited = 1;
    }

    return 0;
}

int progskeet_open(struct progskeet_handle** handle)
{
    struct usb_bus* bus;
    struct usb_device* dev;

    if (!handle)
        return -1;

    dev = NULL;

    usb_find_busses(); /* find all busses */
    usb_find_devices(); /* find all connected devices */

    bus = usb_get_busses();
    while (bus) {
        dev = bus->devices;
        while (dev) {
            if (dev->descriptor.idVendor  == PROGSKEET_USB_VID &&
                dev->descriptor.idProduct == PROGSKEET_USB_PID) {
                /* Easy now, Skeeter */
                break;
            }

            /* We don't take kindly to your kind around here! */

            dev = dev->next;
        }

        bus = bus->next;
    }

    if (!dev)
        return -2;

    return progskeet_open_specific(handle, dev);
}

int progskeet_open_specific(struct progskeet_handle** handle, void* vdev)
{
    struct usb_device* dev;
    struct usb_dev_handle* hdev;

    dev = (struct usb_device*)vdev;

    if (!handle || !dev)
        return -1;

    if (!(hdev = usb_open(dev)))
        return -3;

    if (usb_set_configuration(hdev, PROGSKEET_USB_CFG) < 0)
        return -4;

    if (usb_claim_interface(hdev, PROGSKEET_USB_INT) < 0)
        return -5;

    *handle = (struct progskeet_handle*)malloc(sizeof(struct progskeet_handle));

    (*handle)->hdev = hdev;

    (*handle)->txbuf = (char*)malloc(PROGSKEET_TXBUF_LEN);

    progskeet_reset(*handle);

    return 0;
}

int progskeet_close(struct progskeet_handle* handle)
{
    if (!handle)
        return -1;

    usb_release_interface(USB_HANDLE(handle), 0);

    free(handle->txbuf);

    progskeet_free_rxlist(handle->rxlist);

    free(handle);

    return 0;
}

int progskeet_reset(struct progskeet_handle* handle)
{
    if (!handle)
        return -1;

    handle->txlen = 0;

    progskeet_free_rxlist(handle->rxlist);
    handle->rxlist = NULL;

    handle->cancel = 0;

    handle->cur_addr = 0;
    progskeet_set_addr(handle, 0, 0);

    handle->cur_gpio = 0;
    progskeet_set_gpio(handle, 0);

    handle->cur_gpio_dir = 0;
    progskeet_set_gpio_dir(handle, 0);

    handle->cur_config = 0;
    progskeet_set_config(handle, 10, 1);

    handle->addr_mask = ~0;
    handle->addr_add = 0;

    /* TODO: USB reset */

    return 0;
}

int progskeet_rx(struct progskeet_handle* handle)
{
    int res, ret;
    size_t received;
    struct progskeet_rxloc* rxnext;

    ret = 0;

    /* Compute the total receive size */
    while (handle->rxlist && !handle->cancel && ret == 0) {
        rxnext = handle->rxlist->next;

        received = 0;
        while ((handle->rxlist->len - received) > 0 && !handle->cancel) {
            res = usb_bulk_read(USB_HANDLE(handle),
                                PROGSKEET_USB_EP_IN,
                                handle->rxlist->addr + received,
                                handle->rxlist->len - received,
                                PROGSKEET_USB_TIMEOUT);

            if (res < 0) {
                ret = -2;
                break;
            }

            received += res;
        }

        free(handle->rxlist);

        handle->rxlist = rxnext;
    }

    progskeet_free_rxlist(handle->rxlist);

    return ret;
}

int progskeet_tx(struct progskeet_handle* handle)
{
    int res, ret;
    size_t written;

    if (!handle)
        return -1;

    if (handle->txlen < 1)
        return 0;

    ret = 0;

    written = 0;
    while ((handle->txlen - written) > 0 && !handle->cancel) {
        res = usb_bulk_write(USB_HANDLE(handle),
                             PROGSKEET_USB_EP_OUT,
                             handle->txbuf + written,
                             handle->txlen - written,
                             PROGSKEET_USB_TIMEOUT);

        if (res < 0) {
            ret = -2;
            break;
        }

        written += res;
    }

    handle->txlen = 0;

    return 0;
}

int progskeet_sync(struct progskeet_handle* handle)
{
    int res;

    if ((res = progskeet_tx(handle)) < 0)
        return res;

    return progskeet_rx(handle);
}

int progskeet_cancel(struct progskeet_handle* handle)
{
    if (!handle)
        return -1;

    handle->cancel = 1;

    return 0;
}

int progskeet_enqueue_tx(struct progskeet_handle* handle, char data)
{
    if (handle->txlen + 1 > PROGSKEET_TXBUF_LEN)
        return -1;

    handle->txbuf[handle->txlen++] = data;

    return 0;
}

int progskeet_enqueue_tx_buf(struct progskeet_handle* handle, const char* buf, const size_t len)
{
    if (handle->txlen + len > PROGSKEET_TXBUF_LEN)
        return -1;

    memcpy(handle->txbuf + handle->txlen, buf, len);
    handle->txlen += len;

    return 0;
}

int progskeet_enqueue_rxloc(struct progskeet_handle* handle, void* addr, size_t len)
{
    struct progskeet_rxloc* rxloc;
    struct progskeet_rxloc* rxloci;

    rxloc = (struct progskeet_rxloc*)malloc(sizeof(struct progskeet_rxloc));

    rxloc->next = NULL;
    rxloc->addr = addr;
    rxloc->len = len;

    if (handle->rxlist == NULL) {
        handle->rxlist = rxloc;
        return 0;
    }

    rxloci = handle->rxlist;
    for (;;) {
        if (rxloci->next == NULL) {
            rxloci->next = rxloc;
            return 0;
        }

        rxloci = rxloci->next;
    }
}

int progskeet_set_gpio_dir(struct progskeet_handle* handle, const uint16_t dir)
{
    char cmdbuf[3];
    int res;

    if (!handle)
        return -1;

    if (handle->cur_gpio_dir == dir)
        return 0;

    cmdbuf[0] = PROGSKEET_CMD_SET_GPIO_DIR;
    cmdbuf[1] = (dir & 0x00FF) >> 0;
    cmdbuf[2] = (dir & 0xFF00) >> 8;

    if ((res = progskeet_enqueue_tx_buf(handle, cmdbuf, sizeof(cmdbuf))) < 0)
        return res;

    handle->cur_gpio_dir = dir;

    return 0;
}

int progskeet_set_gpio(struct progskeet_handle* handle, const uint16_t gpio)
{
    char cmdbuf[3];
    int res;

    if (!handle)
        return -1;

    if (handle->cur_gpio == gpio)
        return 0;

    cmdbuf[0] = PROGSKEET_CMD_SET_GPIO;
    cmdbuf[1] = (gpio & 0x00FF) >> 0;
    cmdbuf[2] = (gpio & 0xFF00) >> 8;

    if ((res = progskeet_enqueue_tx_buf(handle, cmdbuf, sizeof(cmdbuf))) < 0)
        return res;

    handle->cur_gpio = gpio;

    return 0;
}

int progskeet_get_gpio(struct progskeet_handle* handle, uint16_t* gpio)
{
    int res;

    if ((res = progskeet_enqueue_tx(handle, PROGSKEET_CMD_GET_GPIO)) < 0)
        return res;

    return progskeet_enqueue_rxloc(handle, gpio, sizeof(uint16_t));
}

int progskeet_wait_gpio(struct progskeet_handle* handle, const uint16_t mask, const uint16_t value)
{
    char cmdbuf[5];

    if (!handle)
        return -1;

    if (!mask)
        return 0;

    cmdbuf[0] = PROGSKEET_CMD_WAIT_GPIO;
    cmdbuf[1] = (value & 0x00FF) >> 0;
    cmdbuf[2] = (value & 0xFF00) >> 8;
    cmdbuf[3] = (mask  & 0x00FF) >> 0;
    cmdbuf[4] = (mask  & 0xFF00) >> 8;

    return progskeet_enqueue_tx_buf(handle, cmdbuf, sizeof(cmdbuf));
}

int progskeet_set_addr(struct progskeet_handle* handle, const uint32_t addr, int auto_incr)
{
    char cmdbuf[4];
    uint32_t maddr;
    int res;

    if (!handle)
        return -1;

    maddr = (addr & handle->addr_mask) | handle->addr_add;
    maddr |= auto_incr ? PROGSKEET_ADDR_AUTO_INC : 0;

    if (handle->cur_addr == maddr)
        return 0;

    cmdbuf[0] = PROGSKEET_CMD_SET_ADDR;
    cmdbuf[1] = (maddr & 0x0000FF) >>  0;
    cmdbuf[2] = (maddr & 0x00FF00) >>  8;
    cmdbuf[3] = (maddr & 0xFF0000) >> 16;

    if ((res = progskeet_enqueue_tx_buf(handle, cmdbuf, sizeof(cmdbuf))) < 0)
        return res;

    handle->cur_addr = maddr;

    return 0;
}

int progskeet_set_config(struct progskeet_handle* handle, const uint8_t delay, const int word)
{
    char cmdbuf[2];
    uint8_t config;
    int res;

    if (!handle)
        return -1;

    config = delay;
    config &= PROGSKEET_CFG_DELAY_MASK;

    if (word)
        config |= PROGSKEET_CFG_WORD;

    if (handle->cur_config == config)
        return 0;

    cmdbuf[0] = PROGSKEET_CMD_SET_CONFIG;
    cmdbuf[1] = config;

    if ((res = progskeet_enqueue_tx_buf(handle, cmdbuf, sizeof(cmdbuf))) < 0)
        return res;

    handle->cur_config = config;

    return 0;
}

int progskeet_write(struct progskeet_handle* handle, const char* buf, const size_t len)
{
    return -1;
}

int progskeet_read(struct progskeet_handle* handle, char* buf, const size_t len)
{
    return -1;
}

int progskeet_write_addr(struct progskeet_handle* handle, uint32_t addr, uint16_t data)
{
    int res;

    if ((res = progskeet_set_addr(handle, addr, 0)) < 0)
        return res;

    return progskeet_write(handle, (char*)&data, sizeof(uint16_t));
}

int progskeet_read_addr(struct progskeet_handle* handle, uint32_t addr, uint16_t *data)
{
    int res;

    if ((res = progskeet_set_addr(handle, addr, 0)) < 0)
        return res;

    return progskeet_read(handle, (char*)data, sizeof(uint16_t));
}
