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

#include <array>
#include <cstdint>

/**
 * @file Eh14Params.hpp
 * @brief The EH14's fixed parameters: timings, volume steps, light thresholds, battery range.
 * @details The menu settings and their defaults are in Eh14Defaults.hpp.
 */
namespace eh14::params
{

// ---------------------------------------------------------------------------------------------------------------
// The display
// ---------------------------------------------------------------------------------------------------------------

/**
 * @brief Whether the digits may flip while the clock is talking.
 * @details With false, a minute that turns over during a time announcement is shown after the announcement, so the
 *          digits match the voice. With true, the display always follows the clock (Application::RunBackground).
 */
inline constexpr bool kFlipDisplayWhileSpeaking = false;

/**
 * @brief How long a segment's coil is driven to flip it, in microseconds (SegmentDriver::Pulse).
 * @warning Do not change it unless you are absolutely sure! Shorter risks a segment that does not flip, longer only
 *          heats the coils and the MOSFETs.
 */
inline constexpr std::uint32_t kSegmentPulseUs = 1250;

/**
 * @brief The pause between two segment flips, in ms.
 * @details Lets the boost converter (U9 on the display board) recover. Unlike @ref kSegmentPulseUs, it may be
 *          changed.
 */
inline constexpr std::uint32_t kSegmentGapMs = 60;

/**
 * @brief Time for the boost converter to reach 19V after power-up, in ms.
 */
inline constexpr std::uint32_t kDisplayPowerUpMs = 1000;

// ---------------------------------------------------------------------------------------------------------------
// Buttons and the menu
// ---------------------------------------------------------------------------------------------------------------

/**
 * @brief A second press of the same button within this time is a bounce and is ignored, in ms.
 */
inline constexpr std::uint32_t kDebounceMs = 250;

/**
 * @brief The menu closes after this long without a MENU or CHANGE press, in ms.
 */
inline constexpr std::uint32_t kMenuTimeoutMs = 60000;

/**
 * @brief In the time items (D, E), how long the hours, the minutes and the blank each show, in ms.
 */
inline constexpr std::uint32_t kMenuTimeStepMs = 2000;

/**
 * @brief How often the battery item (H) measures again, in ms.
 */
inline constexpr std::uint32_t kMenuBatteryRefreshMs = 5000;

/**
 * @brief How long CHANGE press in time mode keeps the display blank, in ms - enough to switch the clock off without the
 *        time showing (Application::BlankDisplay) to play intro on the next startup.
 */
inline constexpr std::uint32_t kDisplayBlankMs = 5000;

// ---------------------------------------------------------------------------------------------------------------
// The clock
// ---------------------------------------------------------------------------------------------------------------

/**
 * @brief The time is read from the RTC at most this often while awake, in ms.
 */
inline constexpr std::uint32_t kClockCheckIntervalMs = 500;

/**
 * @brief The clock sleeps until this second of the minute, then stays awake so the new minute shows within half a
 *        second (Application::Sleep).
 */
inline constexpr std::uint8_t kSleepUntilSecond = 55;

/**
 * @brief Sleeps shorter than this, in seconds, are skipped.
 */
inline constexpr std::uint8_t kMinimumSleepS = 3;

// ---------------------------------------------------------------------------------------------------------------
// Sound
// ---------------------------------------------------------------------------------------------------------------

/**
 * @brief Volume steps A1..A12: each 16-bit sample is multiplied by this into the MAX98357A's 32-bit I2S slot. 65536
 *        is full scale.
 */
inline constexpr std::array<std::uint32_t, 12> kVolumes = {250,   500,   1000,  3000,  6000,  12000,
                                                           16000, 22000, 28000, 36000, 48000, 62000};

/**
 * @brief Alarm repetitions (menu G). 0 repeats until a button is pressed.
 */
inline constexpr std::array<std::uint8_t, 4> kAlarmLoops = {0, 10, 20, 60};

/**
 * @brief The silence between two plays of the alarm sound, in ms.
 */
inline constexpr std::uint32_t kAlarmGapMs = 500;

/**
 * @brief The pause between "time set" and "the current time is", in ms.
 */
inline constexpr std::uint32_t kAnnouncementPauseMs = 300;

// ---------------------------------------------------------------------------------------------------------------
// The light sensor - the silent mode
// ---------------------------------------------------------------------------------------------------------------

/**
 * @brief Silent mode sensitivities (menu F, L1..L8 and H1..H8): at or below this raw light sensor reading (12 bits,
 *        U10 on the display board), the room counts as dark.
 */
inline constexpr std::array<std::uint16_t, 8> kSilentThresholds = {255, 128, 64, 32, 16, 12, 9, 6};

/**
 * @brief Light sensor readings averaged per decision, and the gap between them in ms.
 */
inline constexpr std::uint8_t kSilentReadings = 5;
inline constexpr std::uint32_t kSilentReadingGapMs = 50;

/**
 * @brief Leaving the silent mode takes this many raw counts more light than entering it, so a reading near the
 *        threshold does not toggle it.
 */
inline constexpr std::int32_t kSilentHysteresis = 2;

// ---------------------------------------------------------------------------------------------------------------
// The battery
// ---------------------------------------------------------------------------------------------------------------

/**
 * @brief Raw battery readings shown as 0 and 99 in the menu (12 bits through R7/R8, 3/4 of the cell voltage): about
 *        3.38V and 4.2V.
 */
inline constexpr std::uint16_t kBatteryRawEmpty = 3150;
inline constexpr std::uint16_t kBatteryRawFull = 3905;

/**
 * @brief Below this charge, in percent, the status LED flashes twice after a time announcement.
 */
inline constexpr std::uint8_t kBatteryLowPercent = 20;

} // namespace eh14::params
