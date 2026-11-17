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
 * @class Timebase
 * @brief Milliseconds and microseconds since startup, from SysTick.
 * @details SysTick interrupts every millisecond; microseconds come from its down counter. The count stops while the
 *          clock sleeps (no core clock in standby), so every timeout is time spent awake. It wraps after 49 days;
 *          unsigned differences stay correct across the wrap.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Timebase
{
public:
    Timebase() = delete;

    /**
     * @brief Starts SysTick at 1kHz. Call after Clocks::Init.
     */
    static void Init();

    static std::uint32_t GetMillis()
    {
        return millis_;
    }

    static std::uint32_t GetMicros();

    /**
     * @brief Busy-waits @p us microseconds - only for waits too short to do anything else.
     */
    static void DelayMicros(std::uint32_t us);

    /**
     * @brief The SysTick interrupt's entry point.
     */
    static void HandleTick()
    {
        millis_ = millis_ + 1U;
    }

private:
    static inline volatile std::uint32_t millis_ = 0;
};

} // namespace eh14
