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

#include "hardware/SpiFlash.hpp"

namespace eh14
{

/**
 * @class SoundBank
 * @brief The sounds in the flash: where each starts and how long it is.
 * @details
 * From @ref kHeaderAddress, the flash holds the uploaded bank (SerialProtocol): a big-endian 32-bit header length, a
 * 5-byte entry per sound (its number and its length in bytes, big-endian), then all the sounds in entry order. A
 * sound is 16-bit signed mono, little-endian, at 22050Hz.
 *
 * The numbers are fixed (the constants here): 0..20, 30, 40 and 50 are the numbers, then the words of a time
 * announcement, the menu's sounds, and from @ref kFirstAlarm the alarm sounds, as many as were uploaded.
 *
 * An erased flash, or anything that is not a bank, has a header longer than @ref kMaxHeaderLength: @ref Load returns
 * false and the clock waits for a bank (Application).
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class SoundBank
{
public:
    static constexpr std::uint8_t kIntro = 60;
    static constexpr std::uint8_t kOutro = 61;
    static constexpr std::uint8_t kHours = 70;
    static constexpr std::uint8_t kHours1 = 71;
    static constexpr std::uint8_t kHours2To4 = 72;
    static constexpr std::uint8_t kMinutes = 80;
    static constexpr std::uint8_t kMinutes1 = 81;
    static constexpr std::uint8_t kMinutes2To4 = 82;
    static constexpr std::uint8_t kWarning = 90;
    static constexpr std::uint8_t kTimeSet = 91;
    static constexpr std::uint8_t kTimeCurrent = 92;
    static constexpr std::uint8_t kAlarmSet = 93;
    static constexpr std::uint8_t kAlarmCurrent = 94;
    static constexpr std::uint8_t kDing = 95;
    static constexpr std::uint8_t kDong = 96;
    static constexpr std::uint8_t kFirstAlarm = 100;

    static constexpr std::uint8_t kMaxSounds = 160;

    /**
     * @brief Where the bank starts: right after the settings sector.
     */
    static constexpr std::uint32_t kHeaderAddress = SpiFlash::kSectorSize;

    /**
     * @brief The longest possible header: the length word and 255 entries.
     */
    static constexpr std::uint32_t kMaxHeaderLength = 4 + (255 * 5);

    struct Sound
    {
        std::uint32_t offset = 0;
        std::uint32_t length = 0;
    };

    /**
     * @brief Reads the header and logs it (a host reads these lines back after an upload).
     * @return False if the flash holds no bank.
     */
    bool Load();

    /**
     * @brief Sound @p id; zero offset and length if the bank lacks it.
     */
    Sound Get(std::uint8_t id) const
    {
        return (id < kMaxSounds) ? sounds_[id] : Sound{};
    }

    bool Has(std::uint8_t id) const
    {
        const Sound sound = Get(id);
        return sound.offset != 0U && sound.length != 0U;
    }

    std::uint8_t GetAlarmCount() const
    {
        return alarm_count_;
    }

    /**
     * @brief Logs the flash chip's IDs and size, at the start of both the bank's log and an upload's.
     */
    static void LogFlash();

private:
    std::array<Sound, kMaxSounds> sounds_ = {};
    std::uint8_t alarm_count_ = 0;
};

} // namespace eh14
