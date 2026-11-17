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

#include "hardware/ExternalInterrupts.hpp"

#include "hardware/Clocks.hpp"
#include "hardware/IrqPriority.hpp"
#include "sam.h"

extern "C" void EIC_Handler(void)
{
    eh14::ExternalInterrupts::HandleInterrupt();
}

namespace eh14
{

bool ExternalInterrupts::Init()
{
    PM->APBAMASK.reg |= PM_APBAMASK_EIC;
    Clocks::Connect(GCLK_CLKCTRL_ID_EIC_Val, Clocks::kSlowGenerator);

    EIC->CTRL.reg = EIC_CTRL_SWRST;
    WaitSync();
    return true;
}

void ExternalInterrupts::Attach(std::uint8_t line, Edge edge, bool wakes, Handler handler, void *context)
{
    if (line >= kLineCount)
    {
        return;
    }
    slots_[line] = {handler, context};

    // Eight lines per CONFIG register, four bits each: the sense in the low three, no filter.
    const std::uint32_t shift = (line % 8U) * 4U;
    EIC->CONFIG[line / 8U].reg =
        (EIC->CONFIG[line / 8U].reg & ~(0xFUL << shift)) | (static_cast<std::uint32_t>(edge) << shift);

    if (wakes)
    {
        EIC->WAKEUP.reg |= 1UL << line;
    }
    EIC->INTENSET.reg = 1UL << line;
}

void ExternalInterrupts::Start()
{
    EIC->INTFLAG.reg = EIC->INTFLAG.reg;
    EIC->CTRL.reg = EIC_CTRL_ENABLE;
    WaitSync();

    NVIC_ClearPendingIRQ(EIC_IRQn);
    NVIC_SetPriority(EIC_IRQn, IrqPriority::kEic);
    NVIC_EnableIRQ(EIC_IRQn);
}

void ExternalInterrupts::HandleInterrupt()
{
    const std::uint32_t flags = EIC->INTFLAG.reg & EIC->INTENSET.reg;
    EIC->INTFLAG.reg = flags;
    if ((flags & EIC->WAKEUP.reg) != 0U)
    {
        wake_event_ = true;
    }

    for (std::uint8_t line = 0; line < kLineCount; line++)
    {
        if (((flags >> line) & 1U) != 0U && slots_[line].handler != nullptr)
        {
            slots_[line].handler(slots_[line].context);
        }
    }
}

void ExternalInterrupts::WaitSync()
{
    while (EIC->STATUS.bit.SYNCBUSY)
    {
    }
}

} // namespace eh14
