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
 * @class AnalogInputs
 * @brief Single 12-bit readings of the light sensor and the battery.
 * @details The thresholds in Eh14Params.hpp depend on this setup: reference VDDANA/2 with gain 1/2 (full scale 3V3),
 *          clock / 512, the longest sampling time, one sample, and the first conversion after a channel change
 *          discarded. The factory calibration is loaded from the NVM. The ADC is enabled only during a reading.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class AnalogInputs
{
public:
    bool Init();

    /**
     * @brief Reads ADC channel @p channel (AINn), 0..4095. Takes about 1ms.
     */
    std::uint16_t Read(std::uint8_t channel);

    /**
     * @brief The light sensor, raw.
     */
    std::uint16_t ReadLight();

    /**
     * @brief The battery's charge, 0..99, linear between params::kBatteryRawEmpty and params::kBatteryRawFull.
     */
    std::uint8_t ReadBatteryPercent();

private:
    static constexpr std::uint8_t kSampleTime = 63;

    static void WaitSync();
};

} // namespace eh14
