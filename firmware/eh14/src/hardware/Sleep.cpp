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

#include "hardware/Sleep.hpp"

#include "hardware/Clocks.hpp"
#include "hardware/ExternalInterrupts.hpp"
#include "hardware/IrqPriority.hpp"
#include "sam.h"

extern "C" void RTC_Handler(void)
{
    eh14::Sleep::HandleInterrupt();
}

namespace eh14
{

// In .noinit, which startup does not clear, so the values survive a reset.
__attribute__((section(".noinit"), used)) volatile std::uint32_t Sleep::debug_breadcrumb_;
__attribute__((section(".noinit"), used)) volatile std::uint32_t Sleep::debug_sleep_count_;
__attribute__((section(".noinit"), used)) volatile std::uint32_t Sleep::debug_compare_;
__attribute__((section(".noinit"), used)) volatile std::uint32_t Sleep::debug_count_at_sleep_;
__attribute__((section(".noinit"), used)) volatile std::uint32_t Sleep::debug_count_at_wake_;

bool Sleep::Init()
{
    PM->APBAMASK.reg |= PM_APBAMASK_RTC;
    Clocks::Connect(GCLK_CLKCTRL_ID_RTC_Val, Clocks::kSlowGenerator);

    RTC->MODE0.CTRL.reg = RTC_MODE0_CTRL_SWRST;
    WaitSync();
    // A free-running 32-bit counter at 32768Hz / 32, never cleared; a sleep ends at a compare value.
    RTC->MODE0.CTRL.reg = RTC_MODE0_CTRL_MODE_COUNT32 | RTC_MODE0_CTRL_PRESCALER_DIV32;
    WaitSync();
    RTC->MODE0.CTRL.reg |= RTC_MODE0_CTRL_ENABLE;
    WaitSync();

    NVIC_SetPriority(RTC_IRQn, IrqPriority::kSleepTimer);
    NVIC_EnableIRQ(RTC_IRQn);
    return true;
}

void Sleep::SleepFor(std::uint32_t ms)
{
    const std::uint32_t count = ReadCounter();
    RTC->MODE0.COMP[0].reg = count + SleepCycles(ms);
    WaitSync();
    debug_count_at_sleep_ = count;
    debug_compare_ = count + SleepCycles(ms);
    debug_sleep_count_ = debug_sleep_count_ + 1U;
    debug_breadcrumb_ = 1;
    timer_fired_ = false;
    ExternalInterrupts::ClearWakeEvent();
    RTC->MODE0.INTFLAG.reg = RTC_MODE0_INTFLAG_CMP0;
    RTC->MODE0.INTENSET.reg = RTC_MODE0_INTENSET_CMP0;

    const std::uint32_t systick = SysTick->CTRL;
    SysTick->CTRL = systick & ~SysTick_CTRL_TICKINT_Msk;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    // Interrupts masked between the check and WFI: a wake-up in between stays pending, ends the WFI, and is handled
    // once unmasked.
    while (true)
    {
        __disable_irq();
        if (timer_fired_ || ExternalInterrupts::HasWakeEvent())
        {
            __enable_irq();
            break;
        }
        __DSB();
        __WFI();
        debug_breadcrumb_ = 2;
        __enable_irq();
    }
    debug_breadcrumb_ = 3;

    SysTick->CTRL = systick;
    debug_count_at_wake_ = ReadCounter();
    debug_breadcrumb_ = 4;
    RTC->MODE0.INTENCLR.reg = RTC_MODE0_INTENCLR_CMP0;
}

void Sleep::HandleInterrupt()
{
    if ((RTC->MODE0.INTFLAG.reg & RTC_MODE0_INTFLAG_CMP0) != 0U)
    {
        timer_fired_ = true;
    }
    RTC->MODE0.INTFLAG.reg = RTC->MODE0.INTFLAG.reg;
}

std::uint32_t Sleep::ReadCounter()
{
    RTC->MODE0.READREQ.reg = RTC_READREQ_RREQ;
    WaitSync();
    return RTC->MODE0.COUNT.reg;
}

void Sleep::WaitSync()
{
    while (RTC->MODE0.STATUS.bit.SYNCBUSY)
    {
    }
}

} // namespace eh14
