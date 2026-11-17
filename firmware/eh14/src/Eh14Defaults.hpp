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

/**
 * @file Eh14Defaults.hpp
 * @brief The menu settings on a new clock, or when the settings in the flash are missing or invalid (Settings).
 * @details A setting that picks a value from a table in Eh14Params.hpp is an index into it.
 */
namespace eh14::defaults
{

/**
 * @brief Volume (menu A), an index into params::kVolumes: A6.
 */
inline constexpr std::uint8_t kVolume = 5;

/**
 * @brief Alarm sound (menu B), counted from the first alarm sound of the bank: b01.
 */
inline constexpr std::uint8_t kAlarmSound = 0;

/**
 * @brief Silent mode sensitivity (menu F), 1..8 into params::kSilentThresholds, 0 for off.
 */
inline constexpr std::uint8_t kSilentLevel = 0;

/**
 * @brief Whether the silent mode keeps the clock awake to react at once (H) or checks once in a while (L).
 */
inline constexpr bool kSilentHighPower = false;

/**
 * @brief Alarm repetitions (menu G), an index into params::kAlarmLoops: 20.
 */
inline constexpr std::uint8_t kAlarmLoops = 2;

/**
 * @brief The time the clock starts from after the RTC lost power: 00:00.
 */
inline constexpr std::uint8_t kHourAfterPowerLoss = 0;
inline constexpr std::uint8_t kMinuteAfterPowerLoss = 0;

} // namespace eh14::defaults
