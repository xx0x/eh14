/*
MIT License

Copyright (c) 2026 Vaclav Mach

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @file usb_descriptors.c
 * @brief USB descriptors of the EH14: a CDC serial port.
 * @details "xx0x" / "Casotron EH14" with the chip's serial number, as a composite device with an IAD around the two
 *          CDC interfaces. Without a USB vendor ID of its own, the clock uses VID 0xCAFE (from TinyUSB's examples)
 *          with PID 0xE015; the bootloader has PID 0xE016.
 *
 * The serial number is the chip's 128-bit unique ID in uppercase hex.
 */

#include <stdint.h>
#include <string.h>

#include "sam.h"
#include "tusb.h"

#define USB_VID 0xCAFE
#define USB_PID 0xE015
#define USB_BCD_DEVICE 0x0100

enum
{
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL
};

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT 0x02
#define EPNUM_CDC_IN 0x83

enum
{
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

static tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = USB_BCD_DEVICE,
    .iManufacturer = STRID_MANUFACTURER,
    .iProduct = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

/* Bus powered, 500mA, remote wakeup. */
static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 0, EPNUM_CDC_NOTIF, 16, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

static char const *const string_desc[] = {
    NULL,
    "xx0x",
    "Casotron EH14",
};

/* The four words of the chip's unique ID (datasheet, "Serial Number"). */
static uint32_t const *const unique_id_words[4] = {
    (uint32_t const *)0x0080A00C,
    (uint32_t const *)0x0080A040,
    (uint32_t const *)0x0080A044,
    (uint32_t const *)0x0080A048,
};

static uint16_t desc_str[1 + 32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    size_t count = 0;

    if (index == STRID_LANGID)
    {
        desc_str[1] = 0x0409;
        count = 1;
    }
    else if (index == STRID_SERIAL)
    {
        static char const hex[] = "0123456789ABCDEF";
        for (size_t word = 0; word < 4; word++)
        {
            uint32_t value = *unique_id_words[word];
            for (size_t digit = 0; digit < 8; digit++)
            {
                desc_str[1 + (word * 8) + digit] = (uint16_t)hex[(value >> (28 - (4 * digit))) & 0xF];
            }
        }
        count = 32;
    }
    else if (index < sizeof(string_desc) / sizeof(string_desc[0]))
    {
        char const *str = string_desc[index];
        count = strlen(str);
        if (count > 32)
        {
            count = 32;
        }
        for (size_t i = 0; i < count; i++)
        {
            desc_str[1 + i] = (uint16_t)str[i];
        }
    }
    else
    {
        return NULL;
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return desc_str;
}
