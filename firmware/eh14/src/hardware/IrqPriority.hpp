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

#pragma once

#include <cstdint>

namespace eh14
{

/**
 * @class IrqPriority
 * @brief All interrupt priorities in one place; lower is more urgent. The Cortex-M0+ has four levels.
 * @details SysTick first: the millisecond count must never miss a tick, and its handler is one increment. Then the
 *          EIC: button presses, the RTC alarm and the IR receiver's bits, each a few instructions - an IR bit read
 *          late may already be the next one. The audio DMA only counts played buffers, so a little latency costs
 *          nothing. USB and the RTC wake-up come last. Everything else runs in the main loop (Eh14::Loop and
 *          Eh14::RunBackground).
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class IrqPriority
{
public:
    IrqPriority() = delete;

    static constexpr std::uint32_t kSysTick = 0;
    static constexpr std::uint32_t kEic = 1;
    static constexpr std::uint32_t kAudioDma = 2;
    static constexpr std::uint32_t kUsb = 3;
    static constexpr std::uint32_t kSleepTimer = 3;
};

} // namespace eh14
