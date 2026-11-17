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
 * @class Clocks
 * @brief The clock tree: the 32.768kHz crystal, the 48MHz DFLL locked to it, and the generic clock generators.
 * @details
 * The UF2 bootloader runs crystalless (its DFLL tunes itself against USB) and leaves the crystal (X1) off. @ref Init
 * starts the crystal, locks the DFLL to it (closed loop, 1465 x 32.768kHz = 48.005MHz, within USB's 0.25%) and runs
 * the CPU from it; until then the CPU runs from OSC8M. The crystal gets 2s to start. It keeps running through sleep
 * and every reset except power-on, so @ref Init waits for it only after power-on.
 *
 * Generators:
 * - @ref kMainGenerator: 48MHz - CPU, buses, USB, SERCOMs, ADC.
 * - @ref kSlowGenerator: the crystal's 32.768kHz, running in standby - the DFLL reference, the EIC (it needs a
 *   running clock to see edges during sleep) and the RTC that wakes the clock.
 * - @ref kAudioGenerator: 48MHz / 34 for the I2S bit clock, the closest integer divider to 22050Hz (22061Hz, see
 *   AudioOut).
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Clocks
{
public:
    Clocks() = delete;

    static constexpr std::uint32_t kCpuHz = 48005120;

    static constexpr std::uint8_t kMainGenerator = 0;
    static constexpr std::uint8_t kSlowGenerator = 1;
    static constexpr std::uint8_t kAudioGenerator = 3;

    static constexpr std::uint32_t kAudioDivider = 34;

    /**
     * @brief Starts the crystal (waiting for it after power-on), locks the DFLL and switches the CPU to it.
     */
    static void Init();

    /**
     * @brief Routes @p generator to the generic clock channel @p channel (a GCLK_CLKCTRL_ID value) and enables it.
     */
    static void Connect(std::uint8_t channel, std::uint8_t generator);

private:
    /**
     * @brief The DFLL's multiplier: output = reference x this.
     */
    static constexpr std::uint32_t kDfllMultiplier = 1465;

    /**
     * @brief The crystal's startup time as an XOSC32K STARTUP code: 65536 cycles, 2s.
     */
    static constexpr std::uint32_t kCrystalStartup = 6;

    static void WaitGclkSync();
    static void WaitDfllReady();
    static void SetGenerator(std::uint8_t id, std::uint32_t source, std::uint32_t divider, bool run_in_standby);
};

} // namespace eh14
