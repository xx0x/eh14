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
 * @file startup_samd21.c
 * @brief The vector table and the reset handler of the ATSAMD21G18A.
 * @details
 * The device pack's reset handler skips __libc_init_array(), so no C++ constructor would run; hence this file.
 *
 * The UF2 bootloader sets VTOR before jumping to the application; the reset handler sets it again for a debugger that
 * starts the application directly.
 * @author Vaclav Mach
 * @date 2026-11-17
 */

#include <stdint.h>

#include "sam.h"

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

extern int main(void);
extern void __libc_init_array(void);
void _init(void);
void _fini(void);

void Reset_Handler(void);
void Default_Handler(void);

#define WEAK_HANDLER(name) void name(void) __attribute__((weak, alias("Default_Handler")))

WEAK_HANDLER(NMI_Handler);
WEAK_HANDLER(HardFault_Handler);
WEAK_HANDLER(SVC_Handler);
WEAK_HANDLER(PendSV_Handler);
WEAK_HANDLER(SysTick_Handler);
WEAK_HANDLER(PM_Handler);
WEAK_HANDLER(SYSCTRL_Handler);
WEAK_HANDLER(WDT_Handler);
WEAK_HANDLER(RTC_Handler);
WEAK_HANDLER(EIC_Handler);
WEAK_HANDLER(NVMCTRL_Handler);
WEAK_HANDLER(DMAC_Handler);
WEAK_HANDLER(USB_Handler);
WEAK_HANDLER(EVSYS_Handler);
WEAK_HANDLER(SERCOM0_Handler);
WEAK_HANDLER(SERCOM1_Handler);
WEAK_HANDLER(SERCOM2_Handler);
WEAK_HANDLER(SERCOM3_Handler);
WEAK_HANDLER(SERCOM4_Handler);
WEAK_HANDLER(SERCOM5_Handler);
WEAK_HANDLER(TCC0_Handler);
WEAK_HANDLER(TCC1_Handler);
WEAK_HANDLER(TCC2_Handler);
WEAK_HANDLER(TC3_Handler);
WEAK_HANDLER(TC4_Handler);
WEAK_HANDLER(TC5_Handler);
WEAK_HANDLER(TC6_Handler);
WEAK_HANDLER(TC7_Handler);
WEAK_HANDLER(ADC_Handler);
WEAK_HANDLER(AC_Handler);
WEAK_HANDLER(DAC_Handler);
WEAK_HANDLER(PTC_Handler);
WEAK_HANDLER(I2S_Handler);

typedef void (*Vector)(void);

__attribute__((section(".vectors"), used)) const Vector vector_table[16 + PERIPH_COUNT_IRQn] = {
    (Vector)&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    SVC_Handler,
    0,
    0,
    PendSV_Handler,
    SysTick_Handler,
    PM_Handler,
    SYSCTRL_Handler,
    WDT_Handler,
    RTC_Handler,
    EIC_Handler,
    NVMCTRL_Handler,
    DMAC_Handler,
    USB_Handler,
    EVSYS_Handler,
    SERCOM0_Handler,
    SERCOM1_Handler,
    SERCOM2_Handler,
    SERCOM3_Handler,
    SERCOM4_Handler,
    SERCOM5_Handler,
    TCC0_Handler,
    TCC1_Handler,
    TCC2_Handler,
    TC3_Handler,
    TC4_Handler,
    TC5_Handler,
    TC6_Handler,
    TC7_Handler,
    ADC_Handler,
    AC_Handler,
    DAC_Handler,
    PTC_Handler,
    I2S_Handler,
};

void Reset_Handler(void)
{
    // Copy .data from flash and clear .bss before any C or C++ code runs.
    for (uint32_t *src = &_sidata, *dst = &_sdata; dst < &_edata;)
    {
        *dst++ = *src++;
    }
    for (uint32_t *dst = &_sbss; dst < &_ebss;)
    {
        *dst++ = 0;
    }

    SCB->VTOR = (uint32_t)vector_table & SCB_VTOR_TBLOFF_Msk;

    // Errata 13134: the NVM's manual write default is wrong after reset. Set MANW so a stray write to the flash
    // address space cannot start a page write.
    NVMCTRL->CTRLB.bit.MANW = 1;

    // Give the DMA (audio) priority over the CPU for RAM access, so no audio frame is late.
    DMAC->QOSCTRL.reg = DMAC_QOSCTRL_DQOS_HIGH | DMAC_QOSCTRL_FQOS_HIGH | DMAC_QOSCTRL_WRBQOS_HIGH;

    __libc_init_array();
    main();

    while (1)
    {
    }
}

// __libc_init_array calls these around the constructors; without crt start files there is nothing to do.
void _init(void)
{
}

void _fini(void)
{
}

void Default_Handler(void)
{
    // Unhandled interrupt: stop here; IPSR in a debugger shows which one.
    while (1)
    {
    }
}
