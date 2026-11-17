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
 * @file tusb_config.h
 * @brief TinyUSB configuration for the EH14: a CDC serial port, nothing else.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_TUD_RHPORT 0
#define BOARD_TUD_MAX_SPEED OPT_MODE_FULL_SPEED

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_SAMD21
#endif

#define CFG_TUSB_OS OPT_OS_NONE
#define CFG_TUSB_DEBUG 0

#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

#define CFG_TUD_ENDPOINT0_SIZE 64

#define CFG_TUD_CDC 1
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

/* An upload is paced by USB flow control, so the RX FIFO only has to bridge one page program. */
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 512
#define CFG_TUD_CDC_EP_BUFSIZE 64

#ifdef __cplusplus
}
#endif
