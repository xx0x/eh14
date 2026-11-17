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

#include "hardware/Clocks.hpp"

#include "sam.h"

namespace eh14
{

void Clocks::Init()
{
    // The flash needs one wait state at 48MHz (harmless below). Set before speeding up.
    NVMCTRL->CTRLB.bit.RWS = 1;

    PM->CPUSEL.reg = PM_CPUSEL_CPUDIV_DIV1;
    PM->APBASEL.reg = PM_APBASEL_APBADIV_DIV1;
    PM->APBBSEL.reg = PM_APBBSEL_APBBDIV_DIV1;
    PM->APBCSEL.reg = PM_APBCSEL_APBCDIV_DIV1;

    // Run from OSC8M while the DFLL is reconfigured - the bootloader may have left the CPU on the DFLL.
    SYSCTRL->OSC8M.bit.PRESC = 0;
    SYSCTRL->OSC8M.bit.ONDEMAND = 0;
    SetGenerator(kMainGenerator, GCLK_GENCTRL_SRC_OSC8M, 1, false);

    // Start the crystal unless it already runs (only a power-on resets it). RUNSTDBY keeps it going in sleep for the
    // EIC and the RTC.
    const bool crystal_running =
        SYSCTRL->XOSC32K.bit.ENABLE && ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_XOSC32KRDY) != 0U);
    if (crystal_running)
    {
        SYSCTRL->XOSC32K.bit.RUNSTDBY = 1;
    }
    else
    {
        SYSCTRL->XOSC32K.reg = SYSCTRL_XOSC32K_STARTUP(kCrystalStartup) | SYSCTRL_XOSC32K_XTALEN |
                               SYSCTRL_XOSC32K_EN32K | SYSCTRL_XOSC32K_RUNSTDBY;
        SYSCTRL->XOSC32K.reg |= SYSCTRL_XOSC32K_ENABLE;
        while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_XOSC32KRDY) == 0U)
        {
        }
    }

    SetGenerator(kSlowGenerator, GCLK_GENCTRL_SRC_XOSC32K, 1, true);
    Connect(GCLK_CLKCTRL_ID_DFLL48_Val, kSlowGenerator);

    // The DFLL in closed loop on the crystal. Errata 9905: ONDEMAND must be cleared before configuring the DFLL, so
    // it is written alone first. The factory coarse calibration speeds up the lock.
    SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE;
    WaitDfllReady();
    SYSCTRL->DFLLMUL.reg = SYSCTRL_DFLLMUL_CSTEP(31) | SYSCTRL_DFLLMUL_FSTEP(511) | SYSCTRL_DFLLMUL_MUL(kDfllMultiplier);
    WaitDfllReady();
    std::uint32_t coarse = (*reinterpret_cast<const std::uint32_t *>(FUSES_DFLL48M_COARSE_CAL_ADDR) &
                            FUSES_DFLL48M_COARSE_CAL_Msk) >>
                           FUSES_DFLL48M_COARSE_CAL_Pos;
    if (coarse == 0x3FU)
    {
        coarse = 0x1FU;
    }
    SYSCTRL->DFLLVAL.reg = SYSCTRL_DFLLVAL_COARSE(coarse) | SYSCTRL_DFLLVAL_FINE(512);
    WaitDfllReady();
    SYSCTRL->DFLLCTRL.reg = SYSCTRL_DFLLCTRL_ENABLE | SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_WAITLOCK |
                            SYSCTRL_DFLLCTRL_QLDIS;
    WaitDfllReady();
    while ((SYSCTRL->PCLKSR.reg & (SYSCTRL_PCLKSR_DFLLLCKC | SYSCTRL_PCLKSR_DFLLLCKF)) !=
           (SYSCTRL_PCLKSR_DFLLLCKC | SYSCTRL_PCLKSR_DFLLLCKF))
    {
    }

    SetGenerator(kMainGenerator, GCLK_GENCTRL_SRC_DFLL48M, 1, false);
    SetGenerator(kAudioGenerator, GCLK_GENCTRL_SRC_DFLL48M, kAudioDivider, false);
}

void Clocks::Connect(std::uint8_t channel, std::uint8_t generator)
{
    GCLK->CLKCTRL.reg = static_cast<std::uint16_t>(GCLK_CLKCTRL_ID(channel) | GCLK_CLKCTRL_GEN(generator) |
                                                   GCLK_CLKCTRL_CLKEN);
    WaitGclkSync();
}

void Clocks::WaitGclkSync()
{
    while (GCLK->STATUS.bit.SYNCBUSY)
    {
    }
}

void Clocks::WaitDfllReady()
{
    while ((SYSCTRL->PCLKSR.reg & SYSCTRL_PCLKSR_DFLLRDY) == 0U)
    {
    }
}

void Clocks::SetGenerator(std::uint8_t id, std::uint32_t source, std::uint32_t divider, bool run_in_standby)
{
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(id) | GCLK_GENDIV_DIV(divider);
    WaitGclkSync();
    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(id) | source | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_IDC |
                        (run_in_standby ? GCLK_GENCTRL_RUNSTDBY : 0U);
    WaitGclkSync();
}

} // namespace eh14
