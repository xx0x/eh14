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
 * @class Sleep
 * @brief Standby: the core and its clocks stop until the RTC compare or an EIC line wakes it.
 * @details
 * The SAMD21's RTC counts the crystal at 1024Hz and wakes the clock with a compare match; a button, the DS3231's
 * alarm or the remote control (EIC) wake it sooner. Nothing else ends a sleep: any other interrupt that wakes the
 * core (none in standby, but with a debugger attached the clocks and SysTick keep running) only sends it back to
 * sleep. SysTick's interrupt is off meanwhile, so sleep time never counts in Timebase.
 *
 * Sleep periods are powers of two of 1024Hz cycles, like the watchdog's: the longest that fits the requested time,
 * 16s at most. So toward the end of a minute the clock wakes at 16, 32, 48, 52 and 54s, and the silent mode's L
 * reacts to light within about half a minute.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Sleep
{
public:
    bool Init();

    /**
     * @brief Sleeps for at most @p ms (rounded down to a power-of-two period), or until woken.
     */
    void SleepFor(std::uint32_t ms);

    /**
     * @brief The RTC interrupt's entry point.
     */
    static void HandleInterrupt();

    /**
     * @brief The sleep length for @p ms, in 1024Hz cycles: the SAMD21 watchdog's periods.
     */
    static constexpr std::uint32_t SleepCycles(std::uint32_t ms)
    {
        if (ms >= 16000U)
        {
            return 16384;
        }
        const std::uint32_t cycles = ((ms * 1024U) + 500U) / 1000U;
        std::uint32_t period = 8192;
        while (period > 8U && cycles < period)
        {
            period /= 2U;
        }
        return period;
    }

private:
    static void WaitSync();
    static std::uint32_t ReadCounter();

    static inline volatile bool timer_fired_ = false;

public:
    /**
     * @brief Progress of the last sleep, kept through a reset: 1 asleep, 2 core woken by an interrupt, 3 woken for
     *        good, 4 resumed. The others: the RTC count and compare at the start, and the count at the end.
     */
    static volatile std::uint32_t debug_breadcrumb_;
    static volatile std::uint32_t debug_sleep_count_;
    static volatile std::uint32_t debug_compare_;
    static volatile std::uint32_t debug_count_at_sleep_;
    static volatile std::uint32_t debug_count_at_wake_;
};

static_assert(Sleep::SleepCycles(55000) == 16384);
static_assert(Sleep::SleepCycles(7000) == 4096);
static_assert(Sleep::SleepCycles(3000) == 2048);
static_assert(Sleep::SleepCycles(1) == 8);

} // namespace eh14
