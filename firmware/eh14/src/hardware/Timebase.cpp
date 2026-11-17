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

#include "hardware/Timebase.hpp"

#include "hardware/Clocks.hpp"
#include "hardware/IrqPriority.hpp"
#include "sam.h"

extern "C" void SysTick_Handler(void)
{
    eh14::Timebase::HandleTick();
}

namespace eh14
{

void Timebase::Init()
{
    SysTick_Config(Clocks::kCpuHz / 1000U);
    NVIC_SetPriority(SysTick_IRQn, IrqPriority::kSysTick);
}

std::uint32_t Timebase::GetMicros()
{
    // Read twice: if a tick came in between, the second pair is consistent.
    std::uint32_t ms = millis_;
    std::uint32_t value = SysTick->VAL;
    const std::uint32_t ms_again = millis_;
    if (ms != ms_again)
    {
        ms = ms_again;
        value = SysTick->VAL;
    }
    const std::uint32_t load = SysTick->LOAD + 1U;
    return (ms * 1000U) + (((load - value) * 1000U) / load);
}

void Timebase::DelayMicros(std::uint32_t us)
{
    const std::uint32_t start = GetMicros();
    while ((GetMicros() - start) < us)
    {
    }
}

} // namespace eh14
