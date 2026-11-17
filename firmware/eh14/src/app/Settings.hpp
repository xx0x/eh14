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

#include "Eh14Defaults.hpp"

namespace eh14
{

/**
 * @class Settings
 * @brief The menu settings, stored in the first sector of the flash.
 * @details
 * Six bytes at address 0, in @ref Field order - the layout already in every EH14's flash, so the settings survive a
 * firmware update. A value out of range at load (an erased sector reads 0xFF) falls back to its default
 * (Eh14Defaults.hpp).
 *
 * @ref Save writes only when something differs from the flash: the sector is erased and rewritten, and the menu saves
 * on every exit. Uploading a sound bank erases the whole chip, settings included; they are written again on the next
 * save.
 * @author Vaclav Mach
 * @date 2026-11-17
 */
class Settings
{
public:
    static constexpr std::uint32_t kAddress = 0;

    bool Load(std::uint8_t alarm_count);

    /**
     * @brief Writes the settings to the flash if they differ from what is stored.
     */
    void Save();

    std::uint8_t GetVolume() const
    {
        return volume_;
    }

    void SetVolume(std::uint8_t volume)
    {
        volume_ = volume;
    }

    std::uint8_t GetAlarmSound() const
    {
        return alarm_sound_;
    }

    void SetAlarmSound(std::uint8_t sound)
    {
        alarm_sound_ = sound;
    }

    /**
     * @brief The silent mode's sensitivity, 1..8, or 0 for off.
     */
    std::uint8_t GetSilentLevel() const
    {
        return silent_level_;
    }

    void SetSilentLevel(std::uint8_t level)
    {
        silent_level_ = level;
    }

    bool IsSilentEnabled() const
    {
        return silent_level_ > 0U;
    }

    bool IsSilentHighPower() const
    {
        return silent_high_power_;
    }

    void SetSilentHighPower(bool high_power)
    {
        silent_high_power_ = high_power;
    }

    std::uint8_t GetAlarmLoops() const
    {
        return alarm_loops_;
    }

    void SetAlarmLoops(std::uint8_t loops)
    {
        alarm_loops_ = loops;
    }

    /**
     * @brief Whether the next startup plays the intro; set while CHANGE keeps the display blank in time mode.
     */
    bool IsIntroEnabled() const
    {
        return intro_enabled_;
    }

    void SetIntroEnabled(bool enabled)
    {
        intro_enabled_ = enabled;
    }

private:
    enum class Field : std::uint8_t
    {
        VOLUME,
        ALARM_SOUND,
        SILENT_LEVEL,
        SILENT_HIGH_POWER,
        ALARM_LOOPS,
        INTRO_ENABLED,
        COUNT,
    };

    using Bytes = std::array<std::uint8_t, static_cast<std::size_t>(Field::COUNT)>;

    static constexpr std::size_t Index(Field field)
    {
        return static_cast<std::size_t>(field);
    }

    Bytes ToBytes() const;

    static void LogBytes(const Bytes &bytes);

    std::uint8_t volume_ = defaults::kVolume;
    std::uint8_t alarm_sound_ = defaults::kAlarmSound;
    std::uint8_t silent_level_ = defaults::kSilentLevel;
    bool silent_high_power_ = defaults::kSilentHighPower;
    std::uint8_t alarm_loops_ = defaults::kAlarmLoops;
    bool intro_enabled_ = false;
};

} // namespace eh14
